#include <SFML/Graphics.hpp>
#include "Game.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    Game game(4, 0);

    sf::RenderWindow window(sf::VideoMode({1024u, 768u}), "Ghost Card - 抽鬼牌", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    std::cout << "四人抽鬼牌游戏已启动。玩家位于底部，点击底部手牌区域开始抽牌。\n";

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            game.handleInput(*event);
        }

        if (!game.isGameOver()) {
            game.update();
        }

        game.draw(window);
        window.display();
    }

    return 0;
}
