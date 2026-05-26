#include "Card.h"
#include <filesystem>
#include <iostream>

Card::Card(Rank rank)
    : rank(rank), sprite(nullptr) {
    texturePath = buildTexturePath(rank);
    loadTexture();
}

Rank Card::getRank() const {
    return rank;
}

bool Card::isGhost() const {
    return rank == Rank::Ghost;
}

const std::string& Card::getTexturePath() const {
    return texturePath;
}

std::string Card::buildTexturePath(Rank rank) {
    ensureAssetFolderExists();
    if (rank == Rank::Ghost) {
        return "assets/card_ghost.png"; // 鬼牌贴图路径
    }
    int rankIndex = static_cast<int>(rank);
    return "assets/card_pair_" + std::to_string(rankIndex) + ".png"; // 对应配对牌纹理路径
}

void Card::ensureAssetFolderExists() {
    std::filesystem::create_directories("assets");
}

void Card::createPlaceholderTexture(const std::string& path, Rank rank) {
    // 贴图说明：
    // - assets/card_pair_1.png .. assets/card_pair_12.png 为配对卡牌贴图
    // - assets/card_ghost.png 为鬼牌贴图
    const unsigned width = 180;
    const unsigned height = 250;
    sf::Image image({width, height}, sf::Color::White);

    sf::Color fillColor = sf::Color(220, 220, 220);
    if (rank == Rank::Ghost) {
        fillColor = sf::Color(160, 0, 160);
    } else {
        int rankIndex = static_cast<int>(rank);
        if (rankIndex <= 3) fillColor = sf::Color(180, 60, 60);
        else if (rankIndex <= 6) fillColor = sf::Color(60, 120, 220);
        else if (rankIndex <= 9) fillColor = sf::Color(80, 180, 80);
        else fillColor = sf::Color(220, 180, 60);
    }

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            if (x < 4 || x >= width - 4 || y < 4 || y >= height - 4) {
                image.setPixel({x, y}, sf::Color::Black);
            } else {
                image.setPixel({x, y}, fillColor);
            }
        }
    }

    // 在卡牌角落绘制编号指示
    if (rank != Rank::Ghost) {
        int rankNum = static_cast<int>(rank);
        // 左上角绘制编号标记
        for (int dy = 10; dy <= 25; ++dy) {
            for (int dx = 10; dx <= 25; ++dx) {
                image.setPixel({static_cast<unsigned>(dx), static_cast<unsigned>(dy)}, sf::Color::Black);
            }
        }
        // 右上角绘制编号标记
        for (int dy = 10; dy <= 25; ++dy) {
            for (int dx = width - 25; dx < width - 10; ++dx) {
                image.setPixel({static_cast<unsigned>(dx), static_cast<unsigned>(dy)}, sf::Color::Black);
            }
        }
    }

    if (!image.saveToFile(path)) {
        std::cerr << "Failed to save placeholder texture: " << path << std::endl;
    }
}

void Card::loadTexture() {
    if (!std::filesystem::exists(texturePath)) {
        createPlaceholderTexture(texturePath, rank);
    }
    if (!texture.loadFromFile(texturePath)) {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
    } else {
        sprite = std::make_unique<sf::Sprite>(texture);
        sprite->setScale(sf::Vector2f(0.36f, 0.36f));
    }
}

std::string Card::toString() const {
    if (isGhost()) {
        return "Ghost";
    }
    return "Pair " + std::to_string(static_cast<int>(rank));
}

void Card::draw(sf::RenderWindow& window, float x, float y) {
    if (sprite) {
        sprite->setPosition(sf::Vector2f(x, y));
        window.draw(*sprite);
    }
}
