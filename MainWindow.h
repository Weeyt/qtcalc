#pragma once

#include "CalcEngine.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QKeyEvent;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* e) override;

private:
    void buildUi();

    // ввод
    void insertText(const QString& s);       // numbers, functions, constants (after "=" its looping)
    void insertOperator(QChar op);           // + − × ÷
    void insertPostfix(const QString& s);    // ^ ! % ⁄ ^2
    void reciprocal();
    void backspace();
    void clearAll();
    void equals();

    // отображение
    void refresh();
    void updatePreview();
    void showResult(const calc::Value& v, const QString& exprForHistory);
    void showError(const QString& msg);
    void addHistory(const QString& expr, const QString& result);
    bool canPostfix() const;

    // режимы
    void toggleDeg();
    void toggleInv();
    void toggleMode();
    void toggleHistory();
    void setActive(QPushButton* b, bool on);

    QString m_expr;
    bool m_justEvaluated = false;
    bool m_deg = true;
    bool m_inv = false;
    bool m_fractionMode = true;
    bool m_hasLast = false;
    calc::Value m_lastValue;

    QLabel* m_historyLine = nullptr;
    QLineEdit* m_exprEdit = nullptr;
    QLabel* m_previewLabel = nullptr;

    QPushButton *m_degBtn = nullptr, *m_invBtn = nullptr, *m_modeBtn = nullptr;
    QPushButton *m_sinBtn = nullptr, *m_cosBtn = nullptr, *m_tanBtn = nullptr;
    QPushButton *m_lnBtn = nullptr, *m_logBtn = nullptr;

    QWidget* m_histPanel = nullptr;
    QListWidget* m_histList = nullptr;
};
