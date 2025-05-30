#include "services\GameIdentifier.hh"

#include "ui\viewmodels\UnknownGameViewModel.hh"

#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

static std::array<BYTE, 16> ROM = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
static std::array<BYTE, 16> ROM2 = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
static std::string ROM_HASH = "190c4c105786a2121d85018939108a6c";
static std::wstring KNOWN_HASHES_KEY = L"Hashes";

class GameIdentifierHarness : public GameIdentifier
{
public:
    GSL_SUPPRESS_F6 GameIdentifierHarness() noexcept
        : mockConsoleContext(ConsoleID::NES, L"NES")
    {
    }

    void MockResolveHashResponse(unsigned int nGameId)
    {
        mockServer.HandleRequest<ra::api::ResolveHash>(
            [nGameId](const ra::api::ResolveHash::Request& request, ra::api::ResolveHash::Response& response)
        {
            Assert::AreEqual(ROM_HASH, request.Hash);
            response.Result = ra::api::ApiResult::Success;
            response.GameId = nGameId;
            return true;
        });
    }

    void MockCompatibilityTest(unsigned int nGameId)
    {
        mockServer.HandleRequest<ra::api::ResolveHash>(
            [nGameId](const ra::api::ResolveHash::Request& request, ra::api::ResolveHash::Response& response)
        {
            Assert::AreEqual(ROM_HASH, request.Hash);
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 0U;
            return true;
        });

        mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [nGameId](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            vmUnknownGame.SetSelectedGameId(nGameId);
            vmUnknownGame.SetTestMode(true);
            return ra::ui::DialogResult::OK;
        });
    }

    ra::api::mocks::MockServer mockServer;
    ra::data::context::mocks::MockConsoleContext mockConsoleContext;
    ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
    ra::data::context::mocks::MockGameContext mockGameContext;
    ra::data::context::mocks::MockSessionTracker mockSessionTracker;
    ra::data::context::mocks::MockUserContext mockUserContext;
    ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
    ra::services::mocks::MockConfiguration mockConfiguration;
    ra::ui::mocks::MockDesktop mockDesktop;
    ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
    ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;
    ra::services::mocks::MockLocalStorage mockLocalStorage;

private:
    ra::services::mocks::MockAudioSystem mockAudioSystem;
    ra::services::mocks::MockClock mockClock;
    ra::services::mocks::MockThreadPool mockThreadPool;
};

TEST_CLASS(GameIdentifier_Tests)
{
private:

public:
    TEST_METHOD(TestIdentifyGameKnown)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");
        identifier.MockResolveHashResponse(23U);

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
    }

    TEST_METHOD(TestIdentifyGameNull)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");
        identifier.MockResolveHashResponse(23U);

        Assert::AreEqual(0U, identifier.IdentifyGame(nullptr, ROM.size()));
        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), 0U));

        // matching game ID should only update hash if it's non-zero
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
    }

    TEST_METHOD(TestIdentifyGameUnknownCancel)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockEmulatorContext.MockGameTitle("TestGame");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameUnknownSelected)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockEmulatorContext.MockGameTitle("TestGame");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            vmUnknownGame.SetSelectedGameId(23);
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameUnhashable)
    {
        GameIdentifierHarness identifier;
        ra::data::context::mocks::MockConsoleContext n64Context(N64, L"N64");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Could not identify game."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"No hash was generated for the provided content."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameNotLoggedIn)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Logout();

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Cannot load achievements"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"You must be logged in to load achievements. Please reload the game after logging in."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameNoConsole)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");
        identifier.mockConsoleContext.SetId(ConsoleID::UnknownConsoleID);

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Cannot identify game for unknown console."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameHashChange)
    {
        // calling IdentifyGame with a different block of memory that resolves to the currently
        // loaded game should update the game's hash
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockGameContext.SetGameId(23U);
        identifier.mockGameContext.SetGameHash("HASH");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, identifier.mockGameContext.GetMode());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
    }

    TEST_METHOD(TestIdentifyGameHashChangeCompatibilityMode)
    {
        // calling IdentifyGame with a different block of memory that resolves to the currently
        // loaded game should update the game's hash
        GameIdentifierHarness identifier;
        identifier.MockCompatibilityTest(23U);
        identifier.mockGameContext.SetGameId(23U);
        identifier.mockGameContext.SetGameHash("HASH");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::AreEqual(ra::data::context::GameContext::Mode::CompatibilityTest, identifier.mockGameContext.GetMode());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
    }

    TEST_METHOD(TestIdentifyGameCached)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");
        identifier.MockResolveHashResponse(23U);

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));

        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 32U;
            return true;
        });
        Assert::AreEqual(32U, identifier.IdentifyGame(&ROM2.at(0), ROM2.size()));

        // switching back and forth between hashes should avoid the server request
        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 99U;
            return true;
        });
        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::AreEqual(32U, identifier.IdentifyGame(&ROM2.at(0), ROM2.size()));
    }

    TEST_METHOD(TestIdentifyGameUnknownNotCached)
    {
        GameIdentifierHarness identifier;
        identifier.mockEmulatorContext.MockGameTitle("TestGame");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 0U;
            return true;
        });

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            vmUnknownGame.SetSelectedGameId(23);
            return ra::ui::DialogResult::OK;
        });
        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);

        bDialogShown = false;
        identifier.mockDesktop.ResetExpectedWindows();
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            vmUnknownGame.SetSelectedGameId(32);
            return ra::ui::DialogResult::OK;
        });
        Assert::AreEqual(32U, identifier.IdentifyGame(&ROM2.at(0), ROM2.size()));
        Assert::IsTrue(bDialogShown);

        // switching back to the first unidentified game will show the unknown game dialog again
        bDialogShown = false;
        identifier.mockDesktop.ResetExpectedWindows();
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            vmUnknownGame.SetSelectedGameId(99);
            return ra::ui::DialogResult::OK;
        });
        Assert::AreEqual(99U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
    }

    TEST_METHOD(TestIdentifyHashMultiDiscCompatibility)
    {
        GameIdentifierHarness identifier;
        identifier.mockEmulatorContext.MockGameTitle("TestGame");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response) {
                response.Result = ra::api::ApiResult::Success;
                response.GameId = 0U;
                return true;
            });

        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [](ra::ui::viewmodels::MessageBoxViewModel&) {
                // Play 'TestGame' in compatibility test mode?
                return ra::ui::DialogResult::Yes;
            });

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame) {
                bDialogShown = true;
                vmUnknownGame.SetSelectedGameId(23);
                vmUnknownGame.BeginTest();
                return ra::ui::DialogResult::OK;
            });

        const std::string sHash1 = "0123456789abcdef0123456789abcdef";
        const std::string sHash2 = "abcdef0123456789abcdef0123456789";
        Assert::AreEqual(23U, identifier.IdentifyHash(sHash1));
        Assert::IsTrue(bDialogShown);

        // context is not changed by calling IdentifyGame
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, identifier.mockGameContext.GetMode());
        Assert::AreEqual(0U, identifier.mockGameContext.GameId());

        // context is updated by calling ActivateGame
        identifier.ActivateGame(23U);
        Assert::AreEqual(ra::data::context::GameContext::Mode::CompatibilityTest, identifier.mockGameContext.GetMode());
        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(sHash1, identifier.mockGameContext.GameHash());

        // switching to second unknown disc should prompt to test compatibility
        bDialogShown = false;
        Assert::AreEqual(23U, identifier.IdentifyHash(sHash2));
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual(ra::data::context::GameContext::Mode::CompatibilityTest, identifier.mockGameContext.GetMode());
        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(sHash2, identifier.mockGameContext.GameHash());

        // switching back to the first disc should not prompt to test compatibility
        bDialogShown = false;
        Assert::AreEqual(23U, identifier.IdentifyHash(sHash1));
        Assert::IsFalse(bDialogShown);
        Assert::AreEqual(ra::data::context::GameContext::Mode::CompatibilityTest, identifier.mockGameContext.GetMode());
        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(sHash1, identifier.mockGameContext.GameHash());
    }

    TEST_METHOD(TestActivateGameZeroGameNotLoaded)
    {
        GameIdentifierHarness identifier;
        identifier.ActivateGame(0U);

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
        Assert::AreEqual(0U, identifier.mockSessionTracker.CurrentSessionGameId());
    }

    TEST_METHOD(TestActivateGameZeroGameLoaded)
    {
        GameIdentifierHarness identifier;
        identifier.mockGameContext.SetGameId(23U);
        identifier.mockGameContext.SetGameHash("ABCDEF");
        identifier.mockSessionTracker.BeginSession(23U);
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());

        identifier.ActivateGame(0U);

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
        Assert::AreEqual(0U, identifier.mockSessionTracker.CurrentSessionGameId());
    }

    TEST_METHOD(TestActivateGameNotLoggedIn)
    {
        GameIdentifierHarness identifier;
        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        identifier.ActivateGame(23U);

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::IsTrue(bDialogShown);
        Assert::IsFalse(identifier.mockGameContext.WasLoaded());
    }

    TEST_METHOD(TestActivateGameResetCompatibility)
    {
        GameIdentifierHarness identifier;
        identifier.MockCompatibilityTest(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockUserContext.Initialize("User", "ApiToken");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        identifier.ActivateGame(23U);

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(ra::data::context::GameContext::Mode::CompatibilityTest, identifier.mockGameContext.GetMode());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());
        Assert::IsNull(identifier.mockOverlayManager.GetMessage(1U));
        Assert::IsTrue(identifier.mockGameContext.WasLoaded());

        identifier.MockResolveHashResponse(22U);
        Assert::AreEqual(22U, identifier.IdentifyHash(ROM_HASH));
        identifier.ActivateGame(22U);

        Assert::AreEqual(22U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, identifier.mockGameContext.GetMode());
        Assert::AreEqual(22U, identifier.mockSessionTracker.CurrentSessionGameId());
        Assert::IsNull(identifier.mockOverlayManager.GetMessage(1U));
        Assert::IsTrue(identifier.mockGameContext.WasLoaded());
    }

    TEST_METHOD(TestIdentifyAndActivateGameHardcore)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockUserContext.Initialize("User", "ApiToken");

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());
        Assert::IsNull(identifier.mockOverlayManager.GetMessage(1U));
        Assert::IsTrue(identifier.mockGameContext.WasLoaded());
    }

    TEST_METHOD(TestIdentifyAndActivateGameNull)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);

        identifier.IdentifyAndActivateGame(nullptr, ROM.size());

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
    }

    TEST_METHOD(TestIdentifyAndActivateGameUnknownCancel)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockEmulatorContext.MockGameTitle("TestGame");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            return ra::ui::DialogResult::Cancel;
        });

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(L"TestGame"), identifier.mockGameContext.GameTitle());
        Assert::IsTrue(identifier.mockGameContext.WasLoaded()); // LoadGame(0) still called
    }

    TEST_METHOD(TestIdentifyAndActivateGameUnknownSelected)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockEmulatorContext.MockGameTitle("TestGame");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            vmUnknownGame.SetSelectedGameId(23);
            return ra::ui::DialogResult::OK;
        });

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::IsTrue(bDialogShown);
        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
        Assert::IsTrue(identifier.mockGameContext.WasLoaded());
    }

    TEST_METHOD(TestIdentifyAndActivateGameHashChange)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockGameContext.SetGameId(23U);
        identifier.mockGameContext.SetGameHash("HASH");
        identifier.mockUserContext.Initialize("User", "ApiToken");

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());

        // when IdentifyAndActivate is called with a hash that resolves to the current game,
        // it should still be reloaded.
        Assert::IsTrue(identifier.mockGameContext.WasLoaded());
    }

    TEST_METHOD(TestSaveKnownHashesNone)
    {
        GameIdentifierHarness identifier;
        identifier.SaveKnownHashes();

        Assert::IsFalse(identifier.mockLocalStorage.HasStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY));
    }

    TEST_METHOD(TestSaveKnownHashesNew)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");

        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 32U;
            return true;
        });
        Assert::AreEqual(32U, identifier.IdentifyHash(ROM_HASH));

        identifier.SaveKnownHashes();

        Assert::IsTrue(identifier.mockLocalStorage.HasStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY));
        Assert::AreEqual(ra::StringPrintf("%s=%u\n", ROM_HASH, 32U),
            identifier.mockLocalStorage.GetStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY));
    }

    TEST_METHOD(TestSaveKnownHashesUnchanged)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");
        const auto sFileContents = ra::StringPrintf("%s=%u\ninvalid=0\n", ROM_HASH, 32U);
        identifier.mockLocalStorage.MockStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY, sFileContents);
                            
        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 32U;
            return true;
        });
        Assert::AreEqual(32U, identifier.IdentifyHash(ROM_HASH));

        identifier.SaveKnownHashes();

        // invalid entry would be removed if the file were updated
        Assert::AreEqual(sFileContents,
            identifier.mockLocalStorage.GetStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY));
    }

    TEST_METHOD(TestSaveKnownHashesIDChanged)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Initialize("User", "ApiToken");
        const auto sFileContents = ra::StringPrintf("%s=%u\ninvalid=0\n", ROM_HASH, 32U);
        identifier.mockLocalStorage.MockStoredData(
            ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY, sFileContents);

        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 35U;
            return true;
        });
        Assert::AreEqual(35U, identifier.IdentifyHash(ROM_HASH));

        identifier.SaveKnownHashes();

        // invalid entry will be discarded
        Assert::AreEqual(
            ra::StringPrintf("%s=%u\n", ROM_HASH, 35U),
            identifier.mockLocalStorage.GetStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY));
    }

    TEST_METHOD(TestSaveKnownHashesAdded)
    {
        GameIdentifierHarness identifier;
        const std::string sAlternateHash = "abcdef01234567899876543210fedcba";
        identifier.mockUserContext.Initialize("User", "ApiToken");
        const auto sFileContents = ra::StringPrintf("%s=%u\ninvalid=0\n", sAlternateHash, 32U);
        identifier.mockLocalStorage.MockStoredData(
            ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY, sFileContents);

        identifier.mockServer.HandleRequest<ra::api::ResolveHash>(
            [](const ra::api::ResolveHash::Request&, ra::api::ResolveHash::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.GameId = 35U;
            return true;
        });
        Assert::AreEqual(35U, identifier.IdentifyHash(ROM_HASH));

        identifier.SaveKnownHashes();

        // invalid entry will be discarded
        Assert::AreEqual(
            ra::StringPrintf("%s=%u\n%s=%u\n", ROM_HASH, 35U, sAlternateHash, 32U),
            identifier.mockLocalStorage.GetStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY));
    }

    TEST_METHOD(TestIdentifyGameOfflineUnknown)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Logout();
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Cannot load achievements"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This game was not previously identified and requires a connection to identify it."),
                vmMessageBox.GetMessage());
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
    }

    TEST_METHOD(TestIdentifyGameOffline)
    {
        GameIdentifierHarness identifier;
        identifier.mockUserContext.Logout();
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);

        const auto sFileContents = ra::StringPrintf("%s=%u\n", ROM_HASH, 32U);
        identifier.mockLocalStorage.MockStoredData(ra::services::StorageItemType::HashMapping, KNOWN_HASHES_KEY, sFileContents);

        Assert::AreEqual(32U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsFalse(identifier.mockDesktop.WasDialogShown());
    }
};

} // namespace tests
} // namespace services
} // namespace ra
