// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace Main {
class Session;
} // namespace Main

class UserData;

namespace ProfileCloner {

void cloneProfile(not_null<Main::Session*> session, not_null<UserData*> user);

} // namespace ProfileCloner
