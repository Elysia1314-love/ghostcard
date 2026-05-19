#ifndef CARD_H
#define CARD_H

#include <SFML/Graphics.hpp>
#include <string>
#include <memory>

enum class Rank { One = 1, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Eleven, Twelve, Ghost };

class Card {
public:
    Card(Rank rank);
    ~Card() = default;

    Rank getRank() const;
    bool isGhost() const;
    std::string toString() const;
    const std::string& getTexturePath() const;

    void draw(sf::RenderWindow& window, float x, float y);

private:
    void loadTexture();
    static std::string buildTexturePath(Rank rank);
    static void ensureAssetFolderExists();
    static void createPlaceholderTexture(const std::string& path, Rank rank);

    Rank rank;
    std::string texturePath;
    sf::Texture texture;
    std::unique_ptr<sf::Sprite> sprite;
};

#endif // CARD_H