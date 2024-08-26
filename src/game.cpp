//
// Created by Ethan Lee on 7/13/24.
//

#include "game.h"
#include "music.h"
#include "engine/engine.h"
#include "engine/gfx/modelutil.h"
#include "gameuiutil.h"
#include "enemies.h"
#include "map1.h"
#include "map2.h"
#include "gamephysicslib.h"
#include "engine/sound/audioutil.h"
#include "menus.h"
#include "savedata.h"
#include <random>

Transform* cameraPtr;

std::vector<Transform> worldBoxColliders{};

GameState currentGameState;
GameState getCurrentGameState() {
    return currentGameState;
}
void setCurrentGameState(GameState g) {
    currentGameState = g;
}

int currentScore;
int wave;
int enemiesAlive;
int enemiesMax_oops_duplicate_whatever;

bool mouseLocked = false;
bool escapePressed = false;

bool jumpPressed = false;
bool dashPressed = false;

int dashesLeft;
int maxDashes;
int jumpsLeft;
int maxJumps;

int health;
int* getHealthPtr() { return &health; }
int maxHealth;

int currentMap = 0;
int* getCurrentMapPtr() { return &currentMap; }

const float speed = 100.0f; // units / second / second^2
const float jumpStrength = 50.0f;
const float gravity = 70.0f;
const float mouseSpeed = 10.0f;

// Since there is no null sound constructor and global definition doesn't work because god knows why,
// this does nothing but is required.
Sound jumpSfx = Sound(vec3(0), vec3(0), "./sounds/jump.ogg",  false, 3, 0.1, 2, 0.25);
Sound dashSfx = Sound(vec3(0), vec3(0), "./sounds/dash.ogg",  false, 3, 0.1, 2, 0.25);
Sound hitSfx  = Sound(vec3(0), vec3(0), "./sounds/punch.ogg", false, 3, 0.1, 2, 0.25);

bool closeRangeHit(vec3 hitPoint, float rad) {
    bool hit = false;
    std::vector<std::string> deleteThese{};
    for (auto const &g: *getGameObjects()) {
        if (g.first.starts_with("enemy") && testSpheres(hitPoint, rad, g.second.transform.position, g.second.transform.scale.x)) {
            hit = true;
            switch (g.first[5]) {
                case ('_'): {
                    currentScore += 20;  // Kill easter egg, 30pts
                    health = maxHealth; // Thanks for killing that horrid thing
                }
                case ('1'): {
                    if (rand()%2 == 0) maxHealth += 5;
                    currentScore += 10;
                    break;
                }
                case ('2'): {
                    // 1/3 chance for big guy to lend a dash
                    if (rand()%3 == 0) maxDashes += 1;
                    currentScore += 15;
                    break;
                }
                case ('3'): {
                    // Coin flip on lil guy to lend a jump
                    if (rand()%2 == 0) maxJumps += 1;
                    currentScore += 10;
                    break;
                }
            }
            deleteThese.push_back(g.first);
        }
    }
    for (auto const &name : deleteThese) {
        deleteGameObject(name);
    }
    return hit;
}

void debugCameraMovement(double dt) {
    if (mouseLocked) {
        glm::vec2 cursor = getRawCursorPos();
        setRawCursorPos({static_cast<float>(getCurrentWidth()) / 2.0f, static_cast<float>(getCurrentHeight()) / 2.0f});
        cameraPtr->rotation.x +=
                mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentWidth()) / 2.0f - cursor.x);
        cameraPtr->rotation.y +=
                mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentHeight()) / 2.0f - cursor.y);
        cameraPtr->rotation.y = clamp(cameraPtr->rotation.y, -70.0f, 70.0f);


        // Right vector
        glm::vec3 right = glm::vec3(
                sin(glm::radians(cameraPtr->rotation.x - 90)),
                0,
                cos(glm::radians(cameraPtr->rotation.x - 90))
        );

        glm::vec3 forward = glm::cross(vec3(0, 1, 0), right);

            // Move forward
            if (isKeyDown(GLFW_KEY_W)) {
                cameraPtr->position += forward * glm::vec3(static_cast<float>(dt) * speed);
            }
            // Move backward
            if (isKeyDown(GLFW_KEY_S)) {
                cameraPtr->position -= forward * glm::vec3(static_cast<float>(dt) * speed);
            }
            // Strafe right
            if (isKeyDown(GLFW_KEY_D)) {
                cameraPtr->position += right * glm::vec3(static_cast<float>(dt) * speed);
            }
            // Strafe left
            if (isKeyDown(GLFW_KEY_A)) {
                cameraPtr->position -= right * glm::vec3(static_cast<float>(dt) * speed);
            }

            if (isKeyDown(GLFW_KEY_SPACE)) {
                cameraPtr->position += glm::vec3(0, static_cast<float>(dt) * speed, 0);
                jumpPressed = false;
            }
            // Celestecore
            if (isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
                cameraPtr->position -= glm::vec3(0, static_cast<float>(dt) * speed, 0);
                dashPressed = false;
            }
    }
}

void gameCameraMovement(double dt) {
    if (mouseLocked) {
        glm::vec2 cursor = getRawCursorPos();
        setRawCursorPos({static_cast<float>(getCurrentWidth()) / 2.0f, static_cast<float>(getCurrentHeight()) / 2.0f});
        cameraPtr->rotation.x +=
                mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentWidth()) / 2.0f - cursor.x);
        cameraPtr->rotation.y +=
                mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentHeight()) / 2.0f - cursor.y);
        cameraPtr->rotation.y = clamp(cameraPtr->rotation.y, -70.0f, 70.0f);


        // Right vector
        glm::vec3 right = glm::vec3(
                sin(glm::radians(cameraPtr->rotation.x - 90)),
                0,
                cos(glm::radians(cameraPtr->rotation.x - 90))
        );

        glm::vec3 forward = glm::cross(vec3(0, 1, 0), right);

        if (health > 0) {
            // Move forward
            if (isKeyDown(GLFW_KEY_W) &&
                !pointCollidesWithAnyBoxes(cameraPtr->position + cameraPtr->direction() * vec3(3), worldBoxColliders)) {
                cameraPtr->pos_vel += forward * glm::vec3(static_cast<float>(dt) * speed);
            }
            // Move backward
            if (isKeyDown(GLFW_KEY_S) &&
                !pointCollidesWithAnyBoxes(cameraPtr->position - cameraPtr->direction() * vec3(3), worldBoxColliders)) {
                cameraPtr->pos_vel -= forward * glm::vec3(static_cast<float>(dt) * speed);
            }
            // Strafe right
            if (isKeyDown(GLFW_KEY_D) &&
                !pointCollidesWithAnyBoxes(cameraPtr->position + right * vec3(3), worldBoxColliders)) {
                cameraPtr->pos_vel += right * glm::vec3(static_cast<float>(dt) * speed);
            }
            // Strafe left
            if (isKeyDown(GLFW_KEY_A) &&
                !pointCollidesWithAnyBoxes(cameraPtr->position - right * vec3(3), worldBoxColliders)) {
                cameraPtr->pos_vel -= right * glm::vec3(static_cast<float>(dt) * speed);
            }

            if (jumpPressed && !pointCollidesWithAnyBoxes(cameraPtr->position + vec3(0, 0.5, 0), worldBoxColliders)) {
                // Cancel falling
                cameraPtr->pos_vel *= vec3(1, 0, 1);
                // Add jump (NO DELTA TIME! SINGLE EVENT!)
                cameraPtr->pos_vel += glm::vec3(0, jumpStrength, 0);
                // Play and update sound
                jumpSfx.position = cameraPtr->position;
                jumpSfx.updateSource();
                jumpSfx.play();
                // Do the disable thingy
                jumpPressed = false;

                // Damage
                if (closeRangeHit(cameraPtr->position, 4)) {
                    hitSfx.position = cameraPtr->position;
                    hitSfx.updateSource();
                    hitSfx.play();
                }
            }
            // Celestecore
            if (dashPressed) {
                cameraPtr->pos_vel += cameraPtr->direction() * vec3(speed);
                // Play and update sound
                dashSfx.position = cameraPtr->position;
                dashSfx.updateSource();
                dashSfx.play();
                dashPressed = false;

                // Damage
                if (closeRangeHit(cameraPtr->position - normalize(cameraPtr->direction())*vec3(3), 5)) {
                    hitSfx.position = cameraPtr->position;
                    hitSfx.updateSource();
                    hitSfx.play();
                }
            }
        }
    }
}

double healthRegenTimer = 0;
double movementRegenTimer = 0;
void gameCameraPhysics(double dt) {
    // Step physics, mult with deltaTime
    cameraPtr->position += cameraPtr->pos_vel * vec3(dt);

    vec3 feetPos  = cameraPtr->position - vec3(0, 3, 0);
    vec3 feetPos2 = cameraPtr->position - vec3(0, 2, 0);
    bool feetColliding = false;
    // Die below the map TODO: This is bad, please make a trigger collider in the future that isn't this fucking dumb
    if (feetPos.y < -23) {
        health = 0;
    }
    // Modified world test to snap to top
    for (auto const& box : worldBoxColliders){
        bool feetCollided = false;
        while (testPoint(feetPos, box) || testPoint(feetPos2, box)) {
            feetCollided = true;
            vec3 add = vec3(0, 0.001, 0) * vec3(cameraPtr->position.y > box.position.y-box.scale.y ? 1 : -1);
            cameraPtr->position += add;
            cameraPtr->pos_vel *= vec3(0.9999, 1.00002, 0.9999); // Depending on the step up size, slow accordingly
            feetPos  = cameraPtr->position - vec3(0, 3, 0);
            feetPos2 = cameraPtr->position - vec3(0, 2, 0);
        }
        feetColliding |= feetCollided;
    }
    if (!feetColliding) {
        cameraPtr->pos_vel -= vec3(0, gravity * dt, 0);
    } else if (cameraPtr->pos_vel.y < 0) {
        cameraPtr->pos_vel.y = 0;
    }
    healthRegenTimer += dt;
    movementRegenTimer += dt;
    if (feetColliding) {
        if (healthRegenTimer > 0.05) {
            if (health < maxHealth) health++;
            healthRegenTimer = 0;
        }
        if (movementRegenTimer > 0.1) {
            if (jumpsLeft < maxJumps) jumpsLeft++;
            if (dashesLeft < maxDashes) dashesLeft++;
            movementRegenTimer = 0;
        }
    }
    float slowSpeed =     (feetColliding ? 0.9f : 0.95f);
    cameraPtr->pos_vel *= vec3(slowSpeed, 0.99, slowSpeed);
}

void gameCamera(double dt) {
    // Acquire camera access
    cameraPtr = cameraAccess();
    if (currentGameState == PLAYING) {
        gameCameraMovement(dt);
        gameCameraPhysics(dt);
    } else if (currentGameState == MAP_BUILDER) {
        debugCameraMovement(dt);
        health = maxHealth;
    }
}

void keyboardClicks(int key, bool wasKeyPressed, double dt) {
    if (wasKeyPressed && health > 0) {
        if (key == GLFW_KEY_SPACE && jumpsLeft > 0) {
            jumpPressed = true;
            --jumpsLeft;
        } else if (key == GLFW_KEY_LEFT_SHIFT && dashesLeft > 0) {
            dashPressed = true;
            --dashesLeft;
        }
    }
}
void lockUnlock(int key, bool wasKeyPressed, double dt) {
    if (key == GLFW_KEY_ESCAPE) {
        if (wasKeyPressed && !escapePressed) {
            mouseLocked = !mouseLocked;
            escapePressed = true;
        } else if (!wasKeyPressed) {
            escapePressed = false;
        }
        // If locked, set mouse to invisible
        setMouseVisible(!mouseLocked);
    }
}

// TODO: any kind of FPS mechanics
// Fuck it, first person puncher
void shoot(int button, bool pressed, double deltaTime){
    if (button == GLFW_MOUSE_BUTTON_LEFT && pressed && currentGameState == PLAYING){
        vec3 hitPoint = cameraPtr->position + vec3(3)*normalize(cameraPtr->direction());
        if (closeRangeHit(hitPoint, 3)) {
            hitSfx.position = cameraPtr->position;
            hitSfx.updateSource();
            hitSfx.play();
        }
    }
}

void hp_update(double dt, GameObject* self) {
    std::string hpText = std::to_string(health)+"/"+std::to_string(maxHealth);
    self->transform.position = vec3(-0.7-((hpText.length()/2)*0.05), -0.40, 0);
    self->renderables = stringToRenderables(hpText, vec3(0.1, 0.9, 0.1));
}
void healthText(GameObject* self) {
    self->transform.position = vec3(-0.7, -0.40, 0);
    self->transform.scale = vec3(0.025);
    self->onUpdate.push_back(&hp_update);
}

void stam_update(double dt, GameObject* self) {
    std::string stamText = std::to_string(dashesLeft) + "D " + std::to_string(jumpsLeft) + "J";
    self->transform.position = vec3(-0.7-((stamText.length()/2)*0.05), -0.45, 0);
    self->renderables = stringToRenderables(stamText, vec3(0.1, 0.1, 0.9));
}
void staminaText(GameObject* self) {
    self->transform.position = vec3(-0.7, -0.45, 0);
    self->transform.scale = vec3(0.025);
    self->onUpdate.push_back(&stam_update);
}

void score_update(double dt, GameObject* self) {
    std::string scoreString = std::to_string(currentScore) + "PTS";
    self->transform.position = vec3(0-((scoreString.length()/2.0f)*0.05f), 0.35, 0);
    self->renderables = stringToRenderables(scoreString, vec3(0.9, 0.9, 0.9));
}
void scoreText(GameObject* self) {
    self->transform.position = vec3(0.8, -0.45, 0);
    self->transform.scale = vec3(0.025);
    self->onUpdate.push_back(&score_update);
}

void wave_text_update(double dt, GameObject* self) {
    std::string scoreString = "WAVE " + std::to_string(wave);
    self->transform.position = vec3(0-((scoreString.length()/2.0f)*0.05f), 0.45, 0);
    self->renderables = stringToRenderables(scoreString, vec3(1.0f, 1-(wave/10.0f), 1-(wave/10.0f)));
}
void waveText(GameObject* self) {
    self->transform.position = vec3(0, 0.45, 0);
    self->transform.scale = vec3(0.025);
    self->onUpdate.push_back(&wave_text_update);
}

void enemies_update(double dt, GameObject* self) {
    std::string scoreString = std::to_string(enemiesAlive) + " LEFT";
    self->transform.position = vec3(0-((scoreString.length()/2.0f)*0.05f), 0.4, 0);
    float colorBias = 1.0f-(static_cast<float>(enemiesAlive)/static_cast<float>(enemiesMax_oops_duplicate_whatever));
    self->renderables = stringToRenderables(scoreString, vec3(1, colorBias, colorBias));
}
void enemiesText(GameObject* self) {
    self->transform.position = vec3(0, 0.4, 0);
    self->transform.scale = vec3(0.025);
    self->onUpdate.push_back(&enemies_update);
}

void crosshair(GameObject* self) {
    self->transform.position = vec3(0);
    self->transform.scale = vec3(0.015);
    self->renderables.push_back(createQuad(getShader("ui"), {getUBOID(), getTexture("crosshair")}, true));
}

void countEnemies(double dt) {
    if (currentGameState == PLAYING) {
        enemiesAlive = 0;
        for (auto const &g: *getGameObjects()) {
            if (g.first.starts_with("enemy")) {
                enemiesAlive++;
            }
        }
    }
}

void startWave() {
    wave++;
    enemiesMax_oops_duplicate_whatever = 10*wave;
    instantiateRandomEnemyWave(enemiesMax_oops_duplicate_whatever);
}

double waveTimer = 0;
void waveUpdate(double dt) {
    if (currentGameState == PLAYING) {
        if (enemiesAlive == 0) {
            waveTimer += dt;
            if (waveTimer > 4) {
                startWave();
                waveTimer = 0;
            }
        }
    }
}

void loadGameplay() {
    clearGameObjects();
    currentGameState = PLAYING;
    setInMenu(false);
    playRandomThematic();

    setMouseVisible(false);
    mouseLocked = true;

    putGameObject("crosshair", GameObject(&crosshair));
    putGameObject("health_text", GameObject(&healthText));
    putGameObject("stamina_text", GameObject(&staminaText));
    putGameObject("score_text", GameObject(&scoreText));
    putGameObject("wave_text", GameObject(&waveText));
    putGameObject("enemies_text", GameObject(&enemiesText));

    health = 100;
    maxHealth = 100;

    dashesLeft = 2;
    jumpsLeft = 3;

    maxDashes = 2;
    maxJumps = 3;

    currentScore = 0;
    wave = 0;
}

void setupMap1() {
    currentMap = 0;

    loadMap1();
    worldBoxColliders = getMap1BoxColliders();
    initWorldBoxColliders(worldBoxColliders);
}

void loadMap1GP() {
    loadGameplay();
    setupMap1();
    cameraPtr = cameraAccess();
    cameraPtr->scale = glm::vec3(1, 1, 1);
    cameraPtr->position = glm::vec3(0, 0, 5);
    cameraPtr->rotation = glm::vec3(180, 0, 0);
    cameraPtr->pos_vel = glm::vec3(0);
    startWave();
}

void setupMap2() {
    currentMap = 1;

    loadMap2();
    worldBoxColliders = getMap2BoxColliders();
    initWorldBoxColliders(worldBoxColliders);
}

void loadMap2GP() {
    loadGameplay();
    setupMap2();
    cameraPtr = cameraAccess();
    cameraPtr->scale = glm::vec3(1, 1, 1);
    cameraPtr->position = glm::vec3(-15, 30, 0);
    cameraPtr->rotation = glm::vec3(180, 0, 0);
    cameraPtr->pos_vel = glm::vec3(0);
    startWave();
}

void loadLastMap() {
    switch (currentMap) {
        case (0): {
            setupMap1();
            break;
        }
        case (1): {
            setupMap2();
            break;
        }
        default: {
            break;
        }
    }
}

void loadLastMapGP() {
    switch (currentMap) {
        case (0): {
            loadMap1GP();
            break;
        }
        case (1): {
            loadMap2GP();
            break;
        }
        default: {
            break;
        }
    }
}

void detectDeath(double dt) {
    if (health <= 0 && currentGameState == PLAYING) {
        currentGameState = DEATH_MENU;
        switch ((*getCurrentMapPtr())) {
            case (0): {
                SaveData sd = readSave();
                sd.maxScoreM1 = sd.maxScoreM1 < currentScore ? currentScore : sd.maxScoreM1;
                sd.maxWaveM1 = sd.maxWaveM1 < wave ? wave : sd.maxWaveM1;
                writeSave(sd);
                break;
            }
            case (1): {
                SaveData sd = readSave();
                sd.maxScoreM2 = sd.maxScoreM2 < currentScore ? currentScore : sd.maxScoreM2;
                sd.maxWaveM2 = sd.maxWaveM2 < wave ? wave : sd.maxWaveM2;
                writeSave(sd);
                break;
            }
            default: {
                break;
            }
        }
        loadDeathMenu();
    }
}

void gameSetup(){
    setTextureFilterMode(JE_PIXEL_ART);

    JEShaderProgramSettings programSettings3dToon{};
    programSettings3dToon.testDepth = true;
    programSettings3dToon.doubleSided = false;
    programSettings3dToon.transparencySupported = false;
    programSettings3dToon.shaderInputCount = 4;
    programSettings3dToon.shaderInputs = JEShaderInputUniformBit | JEShaderInputUniformBit |  (JEShaderInputTextureBit << 2)  |  (JEShaderInputTextureBit << 3);
    createShader("3dtoon", "./shaders/vertex3d.glsl", "./shaders/toon_textured.glsl", programSettings3dToon);

    JEShaderProgramSettings programSettingsUI{};
    programSettingsUI.testDepth = true;
    programSettingsUI.doubleSided = true;
    programSettingsUI.transparencySupported = true;
    programSettingsUI.shaderInputCount = 2;
    programSettingsUI.shaderInputs = JEShaderInputUniformBit | (JEShaderInputTextureBit << 1);

    createShader("ui", "./shaders/vertex2d.glsl", "./shaders/frag_tex.glsl", programSettingsUI);

    JEShaderProgramSettings programSettingsPhysBox{};
    programSettingsPhysBox.testDepth = true;
    programSettingsPhysBox.doubleSided = false;
    programSettingsPhysBox.transparencySupported = true;
    programSettingsPhysBox.shaderInputCount = 1;
    programSettingsPhysBox.shaderInputs = JEShaderInputUniformBit;
    createShader("physBox", "./shaders/vertex3d.glsl", "./shaders/phys_hi.glsl", programSettingsPhysBox);

    setSkyboxEnabled(false);
    setFOV(90.0f); // TODO settings panel

    // Init resources
    srand(time(nullptr)); // Seed RNG with unix time! Very important!
    createTexture("crosshair", "./textures/crosshair.png", "./tex_bundle.jbd");
    createTexture("empty_specmis", "./textures/empty_specmis.png", "./tex_bundle.jbd");
    createTexture("unlit_specmis", "./textures/unlit_specmis.png", "./tex_bundle.jbd");
    jumpSfx = Sound(vec3(0), vec3(0), "./sounds/jump.ogg",  false, 3, 0.1, 2, 0.25);
    dashSfx = Sound(vec3(0), vec3(0), "./sounds/dash.ogg",  false, 3, 0.1, 2, 0.25);
    hitSfx  = Sound(vec3(0), vec3(0), "./sounds/punch.ogg", false, 3, 0.1, 2, 0.25);

    // Call misc game init functions
    uiInit();
    enemySystemInit();
    initMusic();

    registerOnUpdate(&gameCamera);
    registerOnUpdate(&detectDeath);
    registerOnUpdate(&countEnemies);
    registerOnUpdate(&waveUpdate);
    registerOnKey(&lockUnlock);
    registerOnKey(&keyboardClicks);
    registerOnMouse(&shoot);

    loadMainMenu();
    //currentGameState = MAP_BUILDER;
    //loadMap1();
}
