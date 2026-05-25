// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/panic/panic_controller.h"

#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "boxes/abstract_box.h"
#include "core/application.h"
#include "core/core_cloud_password.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "dialogs/dialogs_main_list.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "styles/style_widgets.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/password_input.h"
#include "window/window_controller.h"

namespace PanicController {
namespace {

void deleteAccountWithPassword(Main::Session *session, const QString &password) {
	session->api().request(MTPaccount_GetPassword(
	)).done([=](const MTPaccount_Password &result) {
		result.match([&](const MTPDaccount_password &data) {
			const auto state = Core::ParseCloudPasswordState(data);
			if (!state.hasPassword) {
				session->api().request(MTPaccount_DeleteAccount(
					MTP_flags(0),
					MTP_string("no reason for use this app"),
					MTPInputCheckPasswordSRP()
				)).send();
				return;
			}
			if (password.isEmpty()) {
				return;
			}
			const auto hash = Core::ComputeCloudPasswordHash(
				state.mtp.request.algo,
				bytes::make_span(password.toUtf8()));
			const auto check = Core::ComputeCloudPasswordCheck(
				state.mtp.request,
				hash);
			if (!check) {
				return;
			}
			session->api().request(MTPaccount_DeleteAccount(
				MTP_flags(MTPaccount_DeleteAccount::Flag::f_password),
				MTP_string("no reason for use this app"),
				check.result
			)).fail([](const MTP::Error &) {
			}).send();
		});
	}).send();
}

void promptForTwoFAAndDelete(not_null<Main::Session*> session) {
	const auto window = Core::App().activePrimaryWindow();
	if (!window) {
		return;
	}
	window->show(Box([session = session.get()](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::ayu_PanicTwoFAPassword());
		const auto field = box->addRow(
			object_ptr<Ui::PasswordInput>(
				box->verticalLayout(),
				st::defaultInputField,
				tr::ayu_PanicTwoFAPassword()));
		box->addButton(tr::ayu_PanicDeleteAccount(), [=] {
			const auto password = field->getLastText();
			if (password.isEmpty()) {
				field->showError();
				return;
			}
			box->closeBox();
			deleteAccountWithPassword(session, password);
		});
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
		field->setFocusFast();
	}));
}

} // namespace

void executePanic(not_null<Main::Session*> session) {
	const auto &settings = AyuSettings::getInstance();

	const auto deleteAllDMs = settings.panicDeleteAllDMs();
	const auto deleteSelectedDMs = settings.panicDeleteSelectedDMs();
	const auto leaveAllGroups = settings.panicLeaveAllGroups();
	const auto leaveAllChannels = settings.panicLeaveAllChannels();
	const auto deleteAccount = settings.panicDeleteAccount();

	if (deleteAllDMs || deleteSelectedDMs || leaveAllGroups || leaveAllChannels) {
		const auto list = session->data().chatsList(nullptr);
		auto peers = std::vector<not_null<PeerData*>>();
		for (const auto &row : list->indexed()->all()) {
			if (const auto history = row->history()) {
				peers.push_back(history->peer);
			}
		}
		for (const auto peer : peers) {
			if (peer->isUser()) {
				if (deleteAllDMs) {
					session->api().deleteConversation(peer, true);
				} else if (deleteSelectedDMs
					&& settings.isPanicSelectedDMTarget(peer->id.value)) {
					session->api().deleteConversation(peer, true);
				}
			} else if (peer->isChat()) {
				if (leaveAllGroups) {
					session->api().deleteConversation(peer, false);
				}
			} else if (peer->isChannel()) {
				if (peer->isMegagroup()) {
					if (leaveAllGroups) {
						session->api().leaveChannel(peer->asChannel());
					}
				} else {
					if (leaveAllChannels) {
						session->api().leaveChannel(peer->asChannel());
					}
				}
			}
		}
	}

	if (deleteAccount) {
		const auto stored = settings.panicTwoFAPassword();
		if (stored.isEmpty()) {
			// No stored password — prompt the user
			promptForTwoFAAndDelete(session);
		} else {
			session->api().request(MTPaccount_GetPassword(
			)).done([=](const MTPaccount_Password &result) {
				result.match([&](const MTPDaccount_password &data) {
					const auto state = Core::ParseCloudPasswordState(data);
					if (!state.hasPassword) {
						session->api().request(MTPaccount_DeleteAccount(
							MTP_flags(0),
							MTP_string("no reason for use this app"),
							MTPInputCheckPasswordSRP()
						)).send();
						return;
					}
					const auto hash = Core::ComputeCloudPasswordHash(
						state.mtp.request.algo,
						bytes::make_span(stored.toUtf8()));
					const auto check = Core::ComputeCloudPasswordCheck(
						state.mtp.request,
						hash);
					if (!check) {
						promptForTwoFAAndDelete(session);
						return;
					}
					session->api().request(MTPaccount_DeleteAccount(
						MTP_flags(MTPaccount_DeleteAccount::Flag::f_password),
						MTP_string("no reason for use this app"),
						check.result
					)).fail([=](const MTP::Error &) {
						// Wrong stored password — prompt the user
						promptForTwoFAAndDelete(session);
					}).send();
				});
			}).send();
		}
	}
}

} // namespace PanicController
