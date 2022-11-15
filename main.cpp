#include "pch-il2cpp.h"
#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono> 

extern const LPCWSTR LOG_FILE = L"CrabGameHostMod.log";

std::vector<uint64_t> playerIDVector;
std::vector<std::string> playerNameVector;
int GetPlayerIDIndex(std::vector<uint64_t> v, uint64_t id);
void PlayerJoinLeave();

struct Maps
{
    int ID;
    int minPlayers;
    int maxPlayers;
};

// Threads
void GameLoop();
void PlayerLoop();
void GameStateLoop();
void GetInput();
void AFKCheck();

// Function Instances
GameManager* GetGameManager();
LobbyManager* GetLobbyManager();
PlayerInput* GetPlayerInput();
u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3* GetGameLoop();
u109Eu10A6u10A8u10A7u109Au109Au10A2u109Eu10A3u10A0u109F* GetGameModeManager();
GameServer* GetGameServer();

// Functions
uint64_t GetMyID();
uint64_t GetLobbyOwnerID();
//int GetWindowID();
int GetGameModeID();
int GetMapID();
int GetNumOfPlayers();
int GetPlayersAlive();
int GetRandomMapID();
bool GetIsHostAlive();
void SavePosition();
void TeleportToPosition();
void ChangeMap();
void InitializeMapData();
std::string GetLocalTime();
Vector3 GetPlayerRotation(PlayerManager* p, uint64_t ID);

bool canStartGame = true;
bool canResetGame = true;
bool canHostAFK = true;
bool canChangeTime = true;
bool canChangePlayingTime = true;
bool canSkipLobby = true;
bool canChangeMap = true;
bool canSendWinMessage = true;
bool canAFKCheck = true;

bool toggleAutoStart = true;
bool toggleHostAFK = false;
bool toggleSkipLobby = true;
bool toggleSnowballs = false;

bool isPPressed = false;
bool isKPressed = false;
bool isLPressed = false;
bool isOPressed = false;
bool isQPressed = false;
bool isEPressed = false;
bool isMPressed = false;
bool isNPressed = false;

int previousMapID = 0;

enum gameStateEnum { MainMenu, Lobby, Loading, Frozen, Playing, Ended, GameOver };
gameStateEnum GameState = MainMenu;
gameStateEnum prevGameState = MainMenu;

std::vector<uint64_t> deadPlayers;

std::vector<Maps> mapsVector;

Vector3 spawnPos = Vector3({ 0, 0, 0 });
Quaternion playerRot = Quaternion({ 0, 0, 0, 0 });

void Run()
{
    il2cpp_thread_attach(il2cpp_domain_get());

    il2cppi_new_console();
    std::cout << GetLocalTime() << "Mod Started" << std::endl;

    std::thread GameLoopThread(GameLoop);
    std::thread PlayerLoopThread(PlayerLoop);
    std::thread GameStateLoopThread(GameStateLoop);
    std::thread GetInputThread(GetInput);
    std::thread AFKCheckThread(AFKCheck);
    GameLoopThread.detach();
    PlayerLoopThread.detach();
    GameStateLoopThread.detach();
    GetInputThread.detach();
    AFKCheckThread.detach();

    InitializeMapData();
}

int main()
{

}

void GameLoop()
{
    il2cpp_thread_attach(il2cpp_domain_get());

    while (true)
    {
        // Checks if you're the host
        if (GetLobbyOwnerID() == GetMyID() && !LobbyManager_get_practiceMode(GetLobbyManager(), NULL))
        {
            // Start Game
            if (toggleAutoStart && GameState == Lobby && canStartGame)
            {
                if ((toggleHostAFK && GetPlayersAlive() >= 3) || (!toggleHostAFK && GetPlayersAlive() >= 2))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                    ChangeMap();
                    //std::cout << GetLocalTime() << "Game Started" << std::endl;
                    canStartGame = false;
                }
            }

            // Time Change
            if ((GameState == Frozen || GameState == Ended) && canChangeTime && GetGameManager())
            {
                auto gameMode = GetGameManager()->fields.gameMode;
                if (gameMode)
                {
                    if (GameState == Frozen && GameMode_GetFreezeTime(gameMode, NULL) <= 7)
                    {
                        GameMode_SetGameModeTimer(gameMode, 1, 0, NULL); // freeze=0, playing=1, ended=2, gameover=3   
                        //std::cout << GetLocalTime() << "Time Changed" << std::endl;
                        canChangeTime = false;
                    }
                    else if (GameState == Ended && GameMode_GetFreezeTime(gameMode, NULL) <= 5)
                    {
                        //GameMode_SetGameModeTimer(gameMode, 1, 0, NULL); // freeze=0, playing=1, ended=2, gameover=3
                        //std::cout << GetLocalTime() <<  "Time Changed" << std::endl;
                        ChangeMap();
                        //std::cout << GetLocalTime() << "Map Changed" << std::endl;
                        canChangeTime = false;
                    }
                }
            }

            // Change Playing Time
            if (GameState == Playing && canChangePlayingTime)
            {
                if (GetGameManager())
                {
                    auto gameMode = GetGameManager()->fields.gameMode;
                    if (gameMode)
                    {
                        GameMode_SetGameModeTimer(gameMode, 50, 1, NULL); // freeze=0, playing=1, ended=2, gameover=3
                        //std::cout << GetLocalTime() << "Play Time Changed" << std::endl;
                        canChangePlayingTime = false;
                    }
                }
            }

            // Skip Win Screen & Lobby
            if ((canResetGame || canSkipLobby) && GameState == GameOver)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                if (GameState != Loading)
                {
                    if (toggleSkipLobby && !toggleHostAFK && GetNumOfPlayers() >= 2)
                    {
                        ChangeMap();
                        //std::cout << GetLocalTime() << "Skipped Win Screen & Lobby" << std::endl;
                        canSkipLobby = false;
                    }
                    else if (toggleSkipLobby && toggleHostAFK && GetNumOfPlayers() >= 3)
                    {
                        ChangeMap();
                        //std::cout << GetLocalTime() << "Skipped Win Screen & Lobby" << std::endl;
                        canSkipLobby = false;
                    }
                    else if (!toggleSkipLobby)  
                    {
                        u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3_RestartLobby(GetGameLoop(), NULL);
                        //std::cout << GetLocalTime() << "Skipped Win Screen" << std::endl;
                        canResetGame = false;
                    }
                }
            }

            // Host AFK
            /*if (GameState == Playing && toggleHostAFK && canHostAFK && GetIsHostAlive() && GetPlayersAlive() >= 3)
            {
                deadPlayers.push_back(GetMyID());
                auto rb = GetPlayerInput()->fields.playerMovement->fields.u109Au109Du10A4u10A4u10A8u10A5u10A8u10A8u10A6u10A8u10A5;
                auto pos = Rigidbody_get_position(rb, NULL);
                Rigidbody_set_position(rb, Vector3({ pos.x, -100, pos.z }), NULL);
                std::cout << GetLocalTime() <<  "AFKed Yourself" << std::endl;
                canHostAFK = false;
            }*/

            // Host AFK v2
            if (GameState == Frozen && toggleHostAFK && canHostAFK && GetIsHostAlive() && GetPlayersAlive() >= 3)
            {
                auto gameMode = GetGameManager()->fields.gameMode;
                if (gameMode)
                {
                    if (GameMode_GetFreezeTime(gameMode, NULL) <= 9)
                    {
                        deadPlayers.push_back(GetMyID());
                        auto rb = GetPlayerInput()->fields.playerMovement->fields.u109Au109Du10A4u10A4u10A8u10A5u10A8u10A8u10A6u10A8u10A5;
                        auto pos = Rigidbody_get_position(rb, NULL);
                        Rigidbody_set_position(rb, Vector3({ pos.x, -100, pos.z }), NULL);
                        std::cout << GetLocalTime() << "AFKed Yourself" << std::endl;
                        canHostAFK = false;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void PlayerLoop()
{
    il2cpp_thread_attach(il2cpp_domain_get());

    while (true) {

        // Checks if you're the host
        if (GetLobbyOwnerID() == GetMyID() && !LobbyManager_get_practiceMode(GetLobbyManager(), NULL))
        {
            // Player Loop
            auto gameManager = GetGameManager();
            if (gameManager)
            {
                auto activePlayers = gameManager->fields.activePlayers;
                for (size_t i = 0; i < activePlayers->fields.count; i++)
                {
                    if (activePlayers->fields.entries->vector[i].key != 0)
                    {
                        auto playerID = activePlayers->fields.entries->vector[i].key;

                        // Win Message
                        if (canSendWinMessage && GameState == GameOver && GetPlayersAlive() == 1)
                        {
                            if (!activePlayers->fields.entries->vector[i].value->fields.dead)
                            {
                                std::string str = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString() + " WINS!";
                                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                                ServerSend_SendChatMessage(1, (String*)il2cpp_string_new(str.c_str()), NULL);
                                std::cout << GetLocalTime() << str << std::endl;
                                canSendWinMessage = false;
                            }
                        }

                        // Death Messages
                        if (activePlayers->fields.entries->vector[i].value->fields.dead)
                        {
                            if (std::find(deadPlayers.begin(), deadPlayers.end(), playerID) == deadPlayers.end() && GameState != Lobby)
                            {
                                std::string str = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString() + " DIED";
                                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                                ServerSend_SendChatMessage(1, (String*)il2cpp_string_new(str.c_str()), NULL);
                                std::cout << GetLocalTime() << str << std::endl;
                                deadPlayers.push_back(playerID);
                            }
                        }

                        // Remove Snowballs From Inv
                        if (!toggleSnowballs)
                        {
                            GameServer_ForceRemoveItemItemId(playerID, 9, NULL);
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void GameStateLoop()
{
    il2cpp_thread_attach(il2cpp_domain_get());

    while (true)
    {
        // Check GameState
        if (GetGameManager() && GetLobbyManager())
        {
            int intGameState = static_cast<int>(GetGameManager()->fields.gameMode->fields.modeState);
            int intLobbyState = static_cast<int>(GetLobbyManager()->fields.state);

            if (intLobbyState == 0 && GetLobbyOwnerID() == 0 && GameState != MainMenu)
            {
                GameState = MainMenu;
                prevGameState = MainMenu;
                playerIDVector.clear();
                playerNameVector.clear();
            }
            else if (intGameState == 0 && intLobbyState == 2 && GetGameModeID() == 0 && GameState != Lobby)
            {
                GameState = Lobby;
                prevGameState = Lobby;
                canResetGame = true;
                canHostAFK = true;
                canSendWinMessage = true;
                //previousMapID = 0;
                deadPlayers.clear();
            }
            else if (intLobbyState == 1 && GetPlayersAlive() == 0 && GameState != Loading)
            {
                GameState = Loading;
                prevGameState = Loading;
                canChangeTime = true;
                canChangePlayingTime = true;
                canAFKCheck = true;
                canHostAFK = true;
                //canChangeMap = true;
            }
            else if (intGameState == 0 && intLobbyState == 2 && GetGameModeID() != 0 && GetPlayersAlive() >= 2 && prevGameState != Lobby && GameState != Frozen)
            {
                GameState = Frozen;
                prevGameState = Frozen;
                //canHostAFK = true;
                deadPlayers.clear();
            }
            else if (intGameState == 1 && GameState != Playing)
            {
                GameState = Playing;
                prevGameState = Playing;
                canSkipLobby = true;
                canChangeTime = true;
                previousMapID = GetMapID();
                canSendWinMessage = true;
                canResetGame = true;
                canChangeMap = true;
            }
            else if (intGameState == 2 && intLobbyState == 2 && GetPlayersAlive() >= 2 && GameState != Ended)
            {
                GameState = Ended;
                prevGameState = Ended;
            }
            else if (intGameState == 3 && intLobbyState == 4 && GameState != GameOver)
            {
                GameState = GameOver;
                prevGameState = GameOver;
                canStartGame = true;
            }
            //std::cout << GetLocalTime() <<  GameState << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void GetInput()
{
    il2cpp_thread_attach(il2cpp_domain_get());

    while (true)
    {
        // Check If Crab Has Focus
        //if (GetWindowID() == 9)
        
        // Toggle Auto Start
        if (GetAsyncKeyState(VK_LMENU) && GetAsyncKeyState('P') && !isPPressed)
        {
            if (toggleAutoStart)
            {
                toggleAutoStart = false;
                std::cout << GetLocalTime() << "Auto Start OFF" << std::endl;
            }
            else
            {
                toggleAutoStart = true;
                std::cout << GetLocalTime() << "Auto Start ON" << std::endl;
            }
            isPPressed = true;
        }
        else if (!GetAsyncKeyState('P'))
        {
            isPPressed = false;
        }

        // Toggle Host AFk
        if (GetAsyncKeyState(VK_LMENU) && GetAsyncKeyState('K') && !isKPressed)
        {
            if (toggleHostAFK)
            {
                toggleHostAFK = false;
                std::cout << GetLocalTime() << "Host AFK OFF" << std::endl;
            }
            else
            {
                toggleHostAFK = true;
                std::cout << GetLocalTime() << "Host AFK ON" << std::endl;
            }
            isKPressed = true;
        }
        else if (!GetAsyncKeyState('K'))
        {
            isKPressed = false;
        }

        // Toggle Skip Lobby
        if (GetAsyncKeyState(VK_LMENU) && GetAsyncKeyState('L') && !isLPressed)
        {
            if (toggleSkipLobby)
            {
                toggleSkipLobby = false;
                std::cout << GetLocalTime() << "Skip Lobby OFF" << std::endl;
            }
            else
            {
                toggleSkipLobby = true;
                std::cout << GetLocalTime() << "Skip Lobby ON" << std::endl;
            }
            isLPressed = true;
        }
        else if (!GetAsyncKeyState('L'))
        {
            isLPressed = false;
        }

        // Toggle Snowballs
        if (GetAsyncKeyState(VK_LMENU) && GetAsyncKeyState('O') && !isOPressed)
        {
            if (toggleSnowballs)
            {
                toggleSnowballs = false;
                std::cout << GetLocalTime() << "Snowballs OFF" << std::endl;
            }
            else
            {
                toggleSnowballs = true;
                std::cout << GetLocalTime() << "Snowballs ON" << std::endl;
            }
            isOPressed = true;
        }
        else if (!GetAsyncKeyState('O'))
        {
            isOPressed = false;
        }

        // Practice Save Position
        if (GetAsyncKeyState('Q') && !isQPressed)
        {
            SavePosition();
            isQPressed = true;
        }
        else if (!GetAsyncKeyState('Q'))
        {
            isQPressed = false;
        }

        // Practice Teleport To Position
        if (GetAsyncKeyState('E') && !isEPressed)
        {
            TeleportToPosition();
            isEPressed = true;
        }
        else if (!GetAsyncKeyState('E'))
        {
            isEPressed = false;
        }

        // Test
        if (GetAsyncKeyState(VK_LMENU) && GetAsyncKeyState('M') && !isMPressed)
        {
            //ServerSend_TagPlayer(GetMyID(), GetMyID(), NULL);
            //ServerSend_PlayerDamage(GetMySteamID(), GetMySteamID(), 1000, { 0,1,0 }, 4, NULL); // 4 = fall damage

            /*auto gameManager = GetGameManager();
            if (gameManager)
            {
                auto activePlayers = gameManager->fields.activePlayers;

                for (size_t i = 0; i < activePlayers->fields.count; i++)
                {
                    if (activePlayers->fields.entries->vector[i].key != 0)
                    {
                        auto playerID = activePlayers->fields.entries->vector[i].key;
                        auto playerManager = activePlayers->fields.entries->vector[i].value;
                        auto name = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString();
                    }
                }
            }*/

            isMPressed = true;
        }
        else if (!GetAsyncKeyState('M'))
        {
            isMPressed = false;
        }

        // Test
        if (GetAsyncKeyState(VK_LMENU) && GetAsyncKeyState('N') && !isNPressed)
        {
            /*auto gameManager = GetGameManager();
            if (gameManager)
            {
                auto activePlayers = gameManager->fields.activePlayers;
                for (size_t i = 0; i < activePlayers->fields.count; i++)
                {
                    auto playerID = activePlayers->fields.entries->vector[i].key;

                    if (playerID != 0)
                    {
                        auto playerManager = activePlayers->fields.entries->vector[i].value;
                        auto name = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString();

                        auto client = LobbyManager_GetClient(GetLobbyManager(), playerID, NULL);

                        //std::cout << Client_u109Cu10A8u109Eu10A6u10A7u1099u10A1u10A8u10A5u10A8u10A0(client, 9, 0, NULL) << std::endl;
                        //std::cout << Client_u10A4u10A6u109Fu10A0u10A7u10A5u10A6u109Bu10A1u10A5u10A1(client, 9, 0, NULL) << std::endl;
                        //std::cout << Client_u109Cu109Du109Eu10A6u10A7u10A4u10A8u10A2u10A4u109Cu109E(client, 9, 0, NULL) << std::endl;
                        //std::cout << Client_u109Bu109Eu109Du10A3u109Eu1099u109Du109Au1099u109Du10A4(client, 9, 0, NULL) << std::endl;
                        //std::cout << Client_u10A0u109Eu109Bu109Cu109Eu10A4u1099u10A1u109Cu109Cu10A0(client, 9, 0, NULL) << std::endl;
                        //std::cout << Client_u10A7u10A6u1099u10A6u10A7u10A7u10A0u109Eu109Au10A7u1099(client, 9, 0, NULL) << std::endl; // maybe
                        //std::cout << Client_u109Bu109Du10A0u10A2u10A7u10A0u10A7u109Du10A7u109Fu1099_1(client, 9, 0, NULL) << std::endl; // maybe

                        std::cout << name << std::endl;
                        std::cout << client << std::endl << std::endl;

                        //std::cout << client->fields.u10A7u109Fu10A3u109Du1099u10A0u109Du109Au10A2u109Du109E << std::endl; // 1
                        //std::cout << client->fields.u10A1u109Fu10A2u109Bu10A5u10A0u10A2u109Du109Fu10A4u109D << std::endl; // 1
                        //std::cout << client->fields.u10A8u10A1u10A5u109Fu10A1u10A2u109Bu1099u109Fu10A6u10A2 << std::endl; // 1
                        //std::cout << client->fields.u10A4u10A2u109Eu10A1u109Bu109Du10A1u109Fu10A0u109Cu10A6 << std::endl; // 1
                        //std::cout << client->fields.u10A1u10A4u109Au10A5u10A4u10A1u109Eu10A1u109Cu109Du10A8 << std::endl << std::endl; // 1
                    }
                }
            }*/

            //std::cout << "Pressed" << std::endl;

            //auto client = LobbyManager_GetClient(GetLobbyManager(), GetMyID(), NULL);

            //std::cout << client->fields.u10A5u109Bu109Eu1099u109Eu109Cu109Fu109Du109Du10A1u10A8 << std::endl; // 0
            //std::cout << client->fields.u10A7u109Fu10A3u109Du1099u10A0u109Du109Au10A2u109Du109E << std::endl; // 1
            //std::cout << client->fields.u10A1u109Fu10A2u109Bu10A5u10A0u10A2u109Du109Fu10A4u109D << std::endl; // 1
            //std::cout << client->fields.u10A8u10A1u10A5u109Fu10A1u10A2u109Bu1099u109Fu10A6u10A2 << std::endl; // 1
            //std::cout << client->fields.u109Bu10A6u10A3u10A2u109Fu10A4u109Fu10A7u10A6u1099u109B << std::endl; // 0
            //std::cout << client->fields.u10A6u109Du109Du10A8u10A7u10A7u10A3u109Bu10A7u10A0u1099 << std::endl; // 0
            //std::cout << client->fields.u10A4u10A2u109Eu10A1u109Bu109Du10A1u109Fu10A0u109Cu10A6 << std::endl; // 1
            //std::cout << client->fields.u10A1u10A4u109Au10A5u10A4u10A1u109Eu10A1u109Cu109Du10A8 << std::endl << std::endl; // 1

            //std::cout << "1: " << Client_u109Cu10A8u109Eu10A6u10A7u1099u10A1u10A8u10A5u10A8u10A0(client, 10, 0, NULL) << std::endl; // maybe 1
            //std::cout << "2: " << Client_u10A4u10A6u109Fu10A0u10A7u10A5u10A6u109Bu10A1u10A5u10A1(client, 10, 0, NULL) << std::endl; // maybe 1
            //std::cout << "3: " << Client_u10A7u10A6u1099u10A6u10A7u10A7u10A0u109Eu109Au10A7u1099(client, 10, 0, NULL) << std::endl; // maybe 0
            //std::cout << "4: " << Client_u109Bu109Du10A0u10A2u10A7u10A0u10A7u109Du10A7u109Fu1099_1(client, 10, 0, NULL) << std::endl << std::endl; // maybe 0

            //GameServer_ForceGiveWeapon(GetMyID(), 10, 0, NULL);

            // ID 10 = stick

            //auto itemData = u109Fu10A2u10A8u10A1u10A6u1099u10A0u109Du109Cu10A3u10A2_GetItemById(9, NULL);

            //std::cout << " Item ID: " << itemData->fields.itemID << std::endl;
            //std::cout << " Object ID: " << itemData->fields.objectID << std::endl;

            //auto objID = itemData->fields.objectID;
            //(*u109Fu10A2u10A8u10A1u10A6u1099u10A0u109Du109Cu10A3u10A2__TypeInfo)->static_fields->Instance

            isNPressed = true;
        }
        else if (!GetAsyncKeyState('N'))
        {
            isNPressed = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void AFKCheck()
{
    il2cpp_thread_attach(il2cpp_domain_get());

    while (true)
    {
        if (GetLobbyOwnerID() == GetMyID() && !LobbyManager_get_practiceMode(GetLobbyManager(), NULL))
        {
            if (GameState == Playing && canAFKCheck)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << GetLocalTime() << "AFK Check Started" << std::endl;

                std::vector<Vector3> playerRotations;
                std::vector<uint64_t> playerIDVector;

                auto gameManager = GetGameManager();
                if (gameManager)
                {
                    auto activePlayers = gameManager->fields.activePlayers;
                    for (size_t i = 0; i < activePlayers->fields.count; i++)
                    {
                        if (activePlayers->fields.entries->vector[i].key != 0)
                        {
                            auto playerID = activePlayers->fields.entries->vector[i].key;
                            auto playerManager = activePlayers->fields.entries->vector[i].value;

                            Vector3 playerRot = GetPlayerRotation(playerManager, playerID);

                            playerRotations.push_back(playerRot);
                            playerIDVector.push_back(playerID);
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(10));

                    if (GameState == Playing)
                    {
                        for (size_t i = 0; i < activePlayers->fields.count; i++)
                        {
                            if (activePlayers->fields.entries->vector[i].key != 0)
                            {
                                auto playerID = activePlayers->fields.entries->vector[i].key;
                                auto playerManager = activePlayers->fields.entries->vector[i].value; 

                                Vector3 playerRot = GetPlayerRotation(playerManager, playerID);

                                for (int j = 0; j < playerIDVector.size(); j++)
                                {
                                    if (playerID == playerIDVector[j])
                                    {
                                        if (playerRot.x == playerRotations[j].x && playerRot.y == playerRotations[j].y && !activePlayers->fields.entries->vector[i].value->fields.dead)
                                        {
                                            std::string str = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString() + " was killed for being AFK";
                                            ServerSend_SendChatMessage(1, (String*)il2cpp_string_new(str.c_str()), NULL);
                                            deadPlayers.push_back(playerID);
                                            std::cout << GetLocalTime() << str << std::endl;

                                            if (playerID == GetMyID())
                                            {
                                                auto rb = GetPlayerInput()->fields.playerMovement->fields.u109Au109Du10A4u10A4u10A8u10A5u10A8u10A8u10A6u10A8u10A5;
                                                auto pos = Rigidbody_get_position(rb, NULL);
                                                Rigidbody_set_position(rb, Vector3({ pos.x, -100, pos.z }), NULL);
                                            }
                                            else if (playerID != GetMyID())
                                            {
                                                auto rb = playerManager->fields._u1099u109Fu10A1u10A4u10A3u109Bu10A4u10A8u10A7u109Bu1099_k__BackingField->fields.u109Au109Du10A4u10A4u10A8u10A5u10A8u10A8u10A6u10A8u10A5;
                                                auto pos = Rigidbody_get_position(rb, NULL);
                                                ServerSend_RespawnPlayer(playerID, Vector3({ pos.x, -100, pos.z }), NULL);
                                            }   
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                canAFKCheck = false;
                std::cout << GetLocalTime() << "AFK Check Finished" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void SavePosition()
{
    if (LobbyManager_get_practiceMode(GetLobbyManager(), NULL) && GetPlayersAlive() != 0)
    {
        auto rb = GetPlayerInput()->fields.playerMovement->fields.u109Au109Du10A4u10A4u10A8u10A5u10A8u10A8u10A6u10A8u10A5;
        auto camTransform = GetPlayerInput()->fields.playerCam;
        spawnPos = Rigidbody_get_position(rb, NULL);
        playerRot = Transform_get_rotation(camTransform, NULL);
    }
}

void TeleportToPosition()
{
    if (LobbyManager_get_practiceMode(GetLobbyManager(), NULL) && GetPlayersAlive() != 0)
    {
        auto rb = GetPlayerInput()->fields.playerMovement->fields.u109Au109Du10A4u10A4u10A8u10A5u10A8u10A8u10A6u10A8u10A5;
        auto camTransform = GetPlayerInput()->fields.playerCam;
        Rigidbody_set_position(rb, spawnPos, NULL);
        Rigidbody_set_velocity(rb, { 0, 0, 0 }, NULL);
        Transform_set_rotation(camTransform, playerRot, NULL);
    }
}

GameManager* GetGameManager()
{
    return (*GameManager__TypeInfo)->static_fields->Instance;
}

LobbyManager* GetLobbyManager()
{
    return (*LobbyManager__TypeInfo)->static_fields->Instance;
}

PlayerInput* GetPlayerInput()
{
    return (*PlayerInput__TypeInfo)->static_fields->_Instance_k__BackingField;
}

u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3* GetGameLoop()
{
    return (*u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3__TypeInfo)->static_fields->Instance;
}

u109Eu10A6u10A8u10A7u109Au109Au10A2u109Eu10A3u10A0u109F* GetGameModeManager()
{
    return (*u109Eu10A6u10A8u10A7u109Au109Au10A2u109Eu10A3u10A0u109F__TypeInfo)->static_fields->Instance;
}

GameServer* GetGameServer()
{
    return (*GameServer__TypeInfo)->static_fields->Instance;
}

uint64_t GetMyID()
{
    return (*u10A0u10A4u10A8u10A1u10A8u109Au10A8u10A1u109Eu1099u109F__TypeInfo)->static_fields->Instance->fields._u109Du109Au10A3u10A6u10A0u10A8u1099u109Au109Du10A7u1099_k__BackingField.m_SteamID;
}

uint64_t GetLobbyOwnerID()
{
    return (*u10A0u10A4u10A8u10A1u10A8u109Au10A8u10A1u109Eu1099u109F__TypeInfo)->static_fields->Instance->fields.currentLobby.m_SteamID;
}

/*int GetWindowID()
{
    char wnd_title[256];
    HWND hwnd = GetForegroundWindow();
    return GetWindowText(hwnd, wnd_title, sizeof(wnd_title));
}*/

int GetGameModeID()
{
    return GetLobbyManager()->fields.gameMode->fields.id;
}

int GetMapID()
{
    return Map_get_id(GetLobbyManager()->fields.map, NULL);
}

int GetPlayersAlive()
{
    auto gameManager = GetGameManager();
    if (gameManager)
    {
        return GameManager_GetPlayersAlive(gameManager, NULL);
    }
    else
    {
        return 0;
    }
}

int GetNumOfPlayers()
{
    int numOfPlayers = 0;

    auto gameManager = GetGameManager();
    if (gameManager)
    {
        auto activePlayers = gameManager->fields.activePlayers;
        for (size_t i = 0; i < activePlayers->fields.count; i++)
        {
            if (activePlayers->fields.entries->vector[i].key != 0)
            {
                numOfPlayers++;
            }
        }

        auto spectators = gameManager->fields.spectators;
        for (size_t i = 0; i < spectators->fields.count; i++)
        {
            if (spectators->fields.entries->vector[i].key != 0)
            {
                numOfPlayers++;
            }
        }
        return numOfPlayers;
    }
    else return numOfPlayers;
}

bool GetIsHostAlive()
{
    bool isHostAlive = false;

    auto gameManager = GetGameManager();
    if (gameManager)
    {
        auto activePlayers = gameManager->fields.activePlayers;
        for (size_t i = 0; i < activePlayers->fields.count; i++)
        {
            if (activePlayers->fields.entries->vector[i].key != 0)
            {
                if (activePlayers->fields.entries->vector[i].value->fields.playerNumber == 1 && !activePlayers->fields.entries->vector[i].value->fields.dead)
                {
                    isHostAlive = true;
                }
            }
        }
        return isHostAlive;
    }
    else
    {
        return isHostAlive;
    }
}

std::string GetLocalTime()
{
    std::string timeString;
    std::string hour;
    std::string min;
    std::string sec;

    time_t now = time(0);

    tm* local_time = localtime(&now);

    if (local_time->tm_hour <= 9)
    {
        hour = "0" + std::to_string(local_time->tm_hour);
    }
    else
    {
        hour = std::to_string(local_time->tm_hour);
    }

    if (local_time->tm_min <= 9)
    {
        min = "0" + std::to_string(local_time->tm_min);
    }
    else
    {
        min = std::to_string(local_time->tm_min);
    }

    if (local_time->tm_sec <= 9)
    {
        sec = "0" + std::to_string(local_time->tm_sec);
    }
    else
    {
        sec = std::to_string(local_time->tm_sec);
    }

    timeString = hour + ":" + min + ":" + sec + " - ";

    return timeString;
}

Vector3 GetPlayerRotation(PlayerManager* p, uint64_t ID)
{
    if (ID != GetMyID())
    {
        auto xRot = p->fields._u1099u109Fu10A1u10A4u10A3u109Bu10A4u10A8u10A7u109Bu1099_k__BackingField->fields.xRot;
        auto yRot = p->fields._u1099u109Fu10A1u10A4u10A3u109Bu10A4u10A8u10A7u109Bu1099_k__BackingField->fields.yRot;
        return { xRot, yRot, 0 };
    }
    else if (ID == GetMyID())
    {
        return GetPlayerInput()->fields.cameraRot;
    }

}

void ChangeMap()
{
    if (GameState == Lobby)
    {
        u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3_StartGames(GetGameLoop(), NULL);
    }
    else if (GameState == Ended)
    {
        u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3_NextGame(GetGameLoop(), NULL);
    }
    else if (GameState == GameOver)
    {
        u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3_RestartLobby(GetGameLoop(), NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        u10A3u10A2u10A8u1099u10A4u1099u10A3u10A4u109Cu10A1u10A3_StartGames(GetGameLoop(), NULL);
    }

    int randomMapID = GetRandomMapID();

    ServerSend_LoadMap_1(randomMapID, 4, NULL);
}

int GetRandomMapID()
{
    std::vector<int> mapIDVector;
    int randomMapID = 0;

    if (GameState != GameOver)
    {
        for (Maps i : mapsVector)
        {
            if (GetPlayersAlive() >= i.minPlayers && GetPlayersAlive() <= i.maxPlayers)
            {
                mapIDVector.push_back(i.ID);
            }
        }
    }
    else if (GameState == GameOver)
    {
        for (Maps i : mapsVector)
        {
            if (GetNumOfPlayers() >= i.minPlayers && GetNumOfPlayers() <= i.maxPlayers)
            {
                mapIDVector.push_back(i.ID);
            }
        }
    }

    randomMapID = mapIDVector[rand() % mapIDVector.size()];

    while (randomMapID == previousMapID)
    {
        randomMapID = mapIDVector[rand() % mapIDVector.size()];
    }

    return randomMapID;
}

void InitializeMapData()
{
    Maps BitterBeach;
    BitterBeach.ID = 0;
    BitterBeach.minPlayers = 6;
    BitterBeach.maxPlayers = 10;
    mapsVector.push_back(BitterBeach);

    Maps MiniMonke;
    MiniMonke.ID = 35;
    MiniMonke.minPlayers = 1;
    MiniMonke.maxPlayers = 6;
    mapsVector.push_back(MiniMonke);

    Maps SmallBeach;
    SmallBeach.ID = 36;
    SmallBeach.minPlayers = 1;
    SmallBeach.maxPlayers = 6;
    mapsVector.push_back(SmallBeach);

    Maps SmallContainers;
    SmallContainers.ID = 38;
    SmallContainers.minPlayers = 1;
    SmallContainers.maxPlayers = 6;
    mapsVector.push_back(SmallContainers);

    Maps TinyTown;
    TinyTown.ID = 40;
    TinyTown.minPlayers = 1;
    TinyTown.maxPlayers = 6;
    //mapsVector.push_back(TinyTown);

    Maps TinyTown2;
    TinyTown2.ID = 39;
    TinyTown2.minPlayers = 1;
    TinyTown2.maxPlayers = 6;
    mapsVector.push_back(TinyTown2);

    Maps SmallPlayground;
    SmallPlayground.ID = 28;
    SmallPlayground.minPlayers = 15;
    SmallPlayground.maxPlayers = 40;
    mapsVector.push_back(SmallPlayground);

    Maps SuacyStage;
    SuacyStage.ID = 53;
    SuacyStage.minPlayers = 4;
    SuacyStage.maxPlayers = 6;
    //mapsVector.push_back(SuacyStage);

    Maps BigSaloon;
    BigSaloon.ID = 32;
    BigSaloon.minPlayers = 5;
    BigSaloon.maxPlayers = 15;
    mapsVector.push_back(BigSaloon);

    Maps SnowTop;
    SnowTop.ID = 29;
    SnowTop.minPlayers = 6;
    SnowTop.maxPlayers = 15;
    mapsVector.push_back(SnowTop);

    Maps LankyLava;
    LankyLava.ID = 15;
    LankyLava.minPlayers = 6;
    LankyLava.maxPlayers = 15;
    mapsVector.push_back(LankyLava);

    Maps FunkyFields;
    FunkyFields.ID = 7;
    FunkyFields.minPlayers = 8;
    FunkyFields.maxPlayers = 40;
    mapsVector.push_back(FunkyFields);

    Maps IceCrack;
    IceCrack.ID = 10;
    IceCrack.minPlayers = 6;
    IceCrack.maxPlayers = 15;
    mapsVector.push_back(IceCrack);

    Maps Karlson;
    Karlson.ID = 14;
    Karlson.minPlayers = 6;
    Karlson.maxPlayers = 10;
    mapsVector.push_back(Karlson);

    Maps Playground;
    Playground.ID = 18;
    Playground.minPlayers = 6;
    Playground.maxPlayers = 15;
    mapsVector.push_back(Playground);

    Maps ReturnToMonke;
    ReturnToMonke.ID = 20;
    ReturnToMonke.minPlayers = 8;
    ReturnToMonke.maxPlayers = 40;
    mapsVector.push_back(ReturnToMonke);

    Maps Splat;
    Splat.ID = 30;
    Splat.minPlayers = 4;
    Splat.maxPlayers = 15;
    mapsVector.push_back(Splat);

    Maps LavaClimb;
    LavaClimb.ID = 54;
    LavaClimb.minPlayers = 7;
    LavaClimb.maxPlayers = 40;
    mapsVector.push_back(LavaClimb);

    Maps MacaroniMountain;
    MacaroniMountain.ID = 55;
    MacaroniMountain.minPlayers = 7;
    MacaroniMountain.maxPlayers = 40;
    mapsVector.push_back(MacaroniMountain);

    Maps SussySandCastle;
    SussySandCastle.ID = 56;
    SussySandCastle.minPlayers = 7;
    SussySandCastle.maxPlayers = 40;
    mapsVector.push_back(SussySandCastle);

    Maps SussySlope;
    SussySlope.ID = 57;
    SussySlope.minPlayers = 8;
    SussySlope.maxPlayers = 40;
    mapsVector.push_back(SussySlope);

    Maps CrabFields;
    CrabFields.ID = 59;
    CrabFields.minPlayers = 8;
    CrabFields.maxPlayers = 40;
    mapsVector.push_back(CrabFields);
}

int GetPlayerIDIndex(std::vector<uint64_t> v, uint64_t id) 
{
    int index = 0;

    for (uint64_t i : v)
    {
        if (i == id)
        {
            return index;
        }

        index++;
    }

    if (index == v.size())
    {
        return -1;
    }
}

void PlayerJoinLeave()
{
    std::vector<uint64_t> tempPlayerIDVector;

    auto gameManager = GetGameManager();
    if (gameManager)
    {
        auto activePlayers = gameManager->fields.activePlayers;

        for (size_t i = 0; i < activePlayers->fields.count; i++)
        {
            if (activePlayers->fields.entries->vector[i].key != 0)
            {
                auto playerID = activePlayers->fields.entries->vector[i].key;
                auto playerManager = activePlayers->fields.entries->vector[i].value;
                auto name = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString();

                // Joined Message
                if (std::find(playerIDVector.begin(), playerIDVector.end(), playerID) == playerIDVector.end())
                {
                    std::string str = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString() + " joined the server";
                    //ServerSend_SendChatMessage(1, (String*)il2cpp_string_new(str.c_str()), NULL);
                    std::cout << GetLocalTime() << str << std::endl;
                    playerIDVector.push_back(playerID);
                    playerNameVector.push_back(name);
                }

                tempPlayerIDVector.push_back(playerID);
            }
        }

        auto spectators = gameManager->fields.spectators;
        for (size_t i = 0; i < spectators->fields.count; i++)
        {
            if (spectators->fields.entries->vector[i].key != 0)
            {
                auto playerID = activePlayers->fields.entries->vector[i].key;
                auto playerManager = activePlayers->fields.entries->vector[i].value;
                auto name = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString();

                // Joined Message
                if (std::find(playerIDVector.begin(), playerIDVector.end(), playerID) == playerIDVector.end())
                {
                    std::string str = activePlayers->fields.entries->vector[i].value->fields.username->toCPPString() + " joined the server";
                    //ServerSend_SendChatMessage(1, (String*)il2cpp_string_new(str.c_str()), NULL);
                    std::cout << GetLocalTime() << str << std::endl;
                    playerIDVector.push_back(playerID);
                    playerNameVector.push_back(name);
                }

                tempPlayerIDVector.push_back(playerID);
            }
        }

        for (int i = 0; i < playerIDVector.size(); i++)
        {
            if (std::find(tempPlayerIDVector.begin(), tempPlayerIDVector.end(), playerIDVector[i]) == tempPlayerIDVector.end())
            {
                //std::string str = playerNameVector[i] + " left the server";
                //ServerSend_SendChatMessage(1, (String*)il2cpp_string_new(str.c_str()), NULL);
                std::cout << GetLocalTime() << playerNameVector[i] << " left the server" << std::endl;
                int index = GetPlayerIDIndex(playerIDVector, playerIDVector[i]);

                std::cout << GetLocalTime() << "Index: " << index << std::endl;

                if (index != -1)
                {
                    playerIDVector.erase(playerIDVector.begin() + index);
                    playerNameVector.erase(playerNameVector.begin() + index);
                }
            }
        }
    }
}