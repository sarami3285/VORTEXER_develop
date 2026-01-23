#include "MissionDataLoader.h"

bool MissionDataLoader::LoadMission(int id, MissionData& outData) {
    if (id < 100) {
        return LoadFromJson(id, outData);
    }
    else {
        GenerateRandomMission(id, outData);
        return true;
    }
}

bool MissionDataLoader::LoadFromJson(int id, MissionData& outData) {
    std::ifstream file("Assets/Data/MissionData.json");
    if (!file.is_open()) return false;

    json j;
    file >> j;
    file.close();

    if (!j.contains("missions") || !j["missions"].is_array()) return false;

    bool found = false;
    for (auto& m : j["missions"]) {
        if (m.contains("id") && m["id"].get<int>() == id) {
            outData = MissionData();
            outData.id = id;
            outData.name = m.value("name", "Unknown Mission");
            outData.description = m.value("description", "");
            outData.iconPath = m.value("icon_path", "Assets/DefaultMission.png");

            outData.targetType = m.value("target_type", "Elimination");

            auto& r = m["rewards"];
            outData.Rewards.BaseCredits = r.value("BaseCredits", 0);
            outData.Rewards.BonusMultiplier = r.value("BonusMultiplier", 1.0f);

            outData.mapWidth = m.value("map_width", 2000.0f);
            outData.mapHeight = m.value("map_height", 2000.0f);

            auto& tc = m["map_tile_config"];
            outData.mapTileConfig.tileType = tc.value("tile_type", "City");
            outData.mapTileConfig.tileSize = tc.value("tile_size", 540); //540が基準
            outData.mapTileConfig.tilesetTexture = tc.value("tileset_texture", "Assets/MapTile.png");
            outData.mapTileConfig.tileSetRows = tc.value("tile_set_rows", 3);

            // 敵配置データの読み込み
            if (m.contains("enemy_configs") && m["enemy_configs"].is_array()) {
                for (auto& e : m["enemy_configs"]) {
                    EnemyConfig config;
                    config.type = e.value("type", "Standard");
                    config.count = e.value("count", 0);

                    // 通常の敵配置（min_dist等）
                    config.min_dist = static_cast<float>(e.value("min_dist", 100.0));
                    config.max_dist = static_cast<float>(e.value("max_dist", 1000.0));

                    // 固定位置の読み込み
                    if (e.contains("positions") && e["positions"].is_array()) {
                        for (auto& posArr : e["positions"]) {
                            if (posArr.is_array() && posArr.size() >= 2) {
                                config.fixedPositions.push_back(Vector2(posArr[0].get<float>(), posArr[1].get<float>()));
                            }
                        }
                    }



                    // --- ここから艦隊（Formation）用の追加読み込み ---
                    if (config.type == "Formation") {
                        config.formationType = e.value("formation_type", "");
                        config.mainShipType = e.value("main_ship_type", "");
                        config.radiusX = e.value("radius_x", 0.0f);
                        config.radiusY = e.value("radius_y", 0.0f);
                        config.formationRotation = e.value("formation_rotation", 0.0f);

                        if (e.contains("center_position") && e["center_position"].is_array()) {
                            config.centerPosition = Vector2(
                                e["center_position"][0].get<float>(),
                                e["center_position"][1].get<float>()
                            );
                        }

                        // 護衛艦の詳細設定
                        if (e.contains("guard_details") && e["guard_details"].is_array()) {
                            for (auto& g : e["guard_details"]) {
                                GuardDetailConfig gd;
                                gd.guardShipType = g.value("guard_ship_type", "");
                                gd.guardCount = g.value("guard_count", 0);
                                gd.radiusRatio = g.value("radius_ratio", 1.0f);
                                config.guardDetails.push_back(gd);
                            }
                        }
                    }
                    // --- ここまで ---

                    outData.enemyConfigs.push_back(config);
                }
            }

            // 破壊対象データの読み込み　（対象破壊ミッション用）
            if (m.contains("target_configs") && m["target_configs"].is_array()) {
                for (auto& t : m["target_configs"]) {
                    TargetConfig tConfig;
                    tConfig.type = t.value("type", "Target");
                    tConfig.hp = t.value("hp", 100);
                    tConfig.texture = t.value("texture", "");
                    if (t.contains("position") && t["position"].is_array()) {
                        tConfig.position = Vector2(t["position"][0].get<float>(), t["position"][1].get<float>());
                    }
                    outData.targetConfigs.push_back(tConfig);
                }
            }

            found = true;
            break;
        }
    }
    return found;
}

void MissionDataLoader::GenerateRandomMission(int id, MissionData& outData) {
    outData = MissionData();
    outData.id = id;

    srand(static_cast<unsigned int>(id));
    int level = ((id - 100) % 5) + 1;

    std::vector<std::string> types = { "City", "Desert", "Ocean" };
    outData.mapTileConfig.tileType = types[rand() % types.size()];
    outData.mapTileConfig.tileSize = 540;
    outData.mapTileConfig.tilesetTexture = "Assets/MapTile.png";
    outData.mapTileConfig.tileSetRows = 3;

    outData.mapWidth = 3000.0f + (level * 1000.0f);
    outData.mapHeight = 3000.0f + (level * 1000.0f);

    outData.targetType = "Elimination";

    outData.Rewards.BaseCredits = 1000 * level;
    outData.Rewards.BonusMultiplier = 1.0f + (level * 0.2f);

    outData.enemyConfigs.clear();

    // 敵グループの生成
    int groupCount = 3 + level;
    for (int i = 0; i < groupCount; ++i) {
        EnemyConfig group;
        group.type = (rand() % 2 == 0) ? "LowLevelEnemy" : "DroneGuard";
        group.count = 5 + level;
        group.min_dist = 100.0f;
        group.max_dist = 1500.0f;
        
        group.centerPosition = Vector2(
            static_cast<float>(800 + rand() % static_cast<int>(outData.mapWidth - 1600)),
            static_cast<float>(800 + rand() % static_cast<int>(outData.mapHeight - 1600))
        );

        // 陣形設定
        group.formationType = "Circle";
        group.radiusX = 400.0f;
        group.radiusY = 400.0f;

        outData.enemyConfigs.push_back(group);
    }

    outData.name = "Random Mission Lv." + std::to_string(level);
    outData.description = "【UNAUTHORIZED MISSION】\n難易度 Lv." + std::to_string(level) + "\n全敵機を殲滅せよ。";
    outData.iconPath = "Assets/DefaultMission.png";
}