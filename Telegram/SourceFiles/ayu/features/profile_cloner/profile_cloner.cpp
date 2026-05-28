// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/profile_cloner/profile_cloner.h"

#include "api/api_peer_photo.h"
#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "main/main_session.h"

namespace ProfileCloner {

void cloneProfile(not_null<Main::Session*> session, not_null<UserData*> user) {
	const auto &settings = AyuSettings::getInstance();

	if (settings.profileClonerCopyName()) {
		const auto firstName = user->firstName;
		const auto lastName = user->lastName;
		auto flags = MTPaccount_UpdateProfile::Flag::f_first_name
			| MTPaccount_UpdateProfile::Flag::f_last_name;
		session->api().request(MTPaccount_UpdateProfile(
			MTP_flags(flags),
			MTP_string(firstName),
			MTP_string(lastName),
			MTPstring()
		)).send();
	}

	if (settings.profileClonerCopyBio()) {
		const auto about = user->about();
		session->api().request(MTPaccount_UpdateProfile(
			MTP_flags(MTPaccount_UpdateProfile::Flag::f_about),
			MTPstring(),
			MTPstring(),
			MTP_string(about)
		)).send();
	}

	if (settings.profileClonerCopyAvatar() && user->hasUserpic()) {
		auto view = user->createUserpicView();
		const auto image = user->userpicCloudImage(view);
		if (image && !image->isNull()) {
			session->api().peerPhoto().upload(
				session->user(),
				Api::PeerPhoto::UserPhoto{.image = *image});
		}
	}
}

} // namespace ProfileCloner
