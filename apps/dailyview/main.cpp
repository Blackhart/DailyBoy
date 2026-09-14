#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "Model/sequence_model.hpp"

int main(int argc, char* argv[]) {
  QGuiApplication app(argc, argv);

  dailyview::SequenceModel sequence_model;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty(QStringLiteral("sequenceModel"),
                                           &sequence_model);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.loadFromModule("DailyBoy.DailyView", "Main");

  return app.exec();
}
