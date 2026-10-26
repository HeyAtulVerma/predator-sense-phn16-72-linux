#include "Translator.h"
#include <QCoreApplication>
#include <cassert>
#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    AppTranslator translator;

    // By default, English translation is active
    assert(translator.isActive());

    // Test essential translations
    assert(translator.translate(nullptr, "Início") == QStringLiteral("Home"));
    assert(translator.translate(nullptr, "Ventoinhas") == QStringLiteral("Fans"));
    assert(translator.translate(nullptr, "Iluminação") == QStringLiteral("Lighting"));
    assert(translator.translate(nullptr, "Monitorização") == QStringLiteral("Monitoring"));
    assert(translator.translate(nullptr, "Definições") == QStringLiteral("Settings"));
    assert(translator.translate(nullptr, "Silencioso") == QStringLiteral("Quiet"));
    assert(translator.translate(nullptr, "Equilibrado") == QStringLiteral("Balanced"));
    assert(translator.translate(nullptr, "Desempenho") == QStringLiteral("Performance"));
    assert(translator.translate(nullptr, "Automático") == QStringLiteral("Auto"));
    assert(translator.translate(nullptr, "Máximo") == QStringLiteral("Max"));
    assert(translator.translate(nullptr, "Personalizado") == QStringLiteral("Custom"));
    assert(translator.translate(nullptr, "Idioma") == QStringLiteral("Language"));

    // When inactive (Portuguese), it returns empty QString so Qt falls back to source text
    translator.setActive(false);
    assert(!translator.isActive());
    assert(translator.translate(nullptr, "Ventoinhas").isEmpty());
    assert(translator.translate(nullptr, "Início").isEmpty());

    // Reactivate English
    translator.setActive(true);
    assert(translator.translate(nullptr, "Ventoinhas") == QStringLiteral("Fans"));

    std::cout << "All translation tests passed successfully!\n";
    return 0;
}
