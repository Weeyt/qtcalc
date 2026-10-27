#include "MainWindow.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <functional>

using namespace calc;

namespace {

const QStringList kFuncPrefixes = {"asin(", "acos(", "atan(", "sin(", "cos(", "tan(", "ln(", "log(", "√("};

bool isBinaryOp(QChar c) {
    return c == '+' || c == sym::Minus || c == sym::Times || c == sym::Divide;
}

const char* kStyle = R"(
QWidget { background:#161618; color:#f2f2f7; font-family:"Segoe UI","Roboto","Helvetica Neue",sans-serif; }
QLabel { background:transparent; }
QLineEdit { background:transparent; border:none; font-size:38px; color:#ffffff; selection-background-color:#4c8dff; }
QLabel#history { color:#8e8e93; font-size:16px; }
QLabel#preview { color:#8e8e93; font-size:20px; }
QPushButton { border:none; border-radius:12px; font-size:20px; background:#2c2c2e; color:#f2f2f7; padding:6px; }
QPushButton:hover { background:#3a3a3c; }
QPushButton:pressed { background:#54545a; }
QPushButton[role="sci"] { background:#202023; color:#c7c7cc; font-size:16px; }
QPushButton[role="sci"]:hover { background:#2a2a2e; }
QPushButton[role="sci"][active="true"] { background:#2f5d3a; color:#d6ffe0; }
QPushButton[role="op"] { background:#2f4f7f; color:#d6e8ff; font-size:24px; }
QPushButton[role="op"]:hover { background:#38609a; }
QPushButton[role="clear"] { background:#5a2d2d; color:#ffb3b3; }
QPushButton[role="eq"] { background:#4c8dff; color:#ffffff; font-size:28px; }
QPushButton[role="eq"]:hover { background:#6aa0ff; }
QPushButton[role="flat"] { background:transparent; color:#8e8e93; font-size:14px; }
QPushButton[role="flat"]:hover { color:#ffffff; }
QListWidget { background:#1c1c1e; border:none; border-radius:10px; font-size:14px; padding:4px; }
QListWidget::item { padding:8px 6px; border-bottom:1px solid #2c2c2e; }
QListWidget::item:selected { background:#2f4f7f; }
)";

} // namespace

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("Калькулятор"));
    setMinimumSize(440, 660);
    resize(460, 700);
    setFocusPolicy(Qt::StrongFocus);
    buildUi();
    refresh();
}

// ------------------------------------------------------------------ UI
void MainWindow::buildUi() {
    setStyleSheet(QString::fromUtf8(kStyle));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(14, 10, 14, 14);
    root->setSpacing(12);

    auto* left = new QVBoxLayout;
    left->setSpacing(6);
    root->addLayout(left, 1);

    // верхняя панель
    auto* top = new QHBoxLayout;
    top->addStretch(1);
    auto* histBtn = new QPushButton(QStringLiteral("🕘 История"));
    histBtn->setProperty("role", "flat");
    histBtn->setFocusPolicy(Qt::NoFocus);
    connect(histBtn, &QPushButton::clicked, this, [this] { toggleHistory(); });
    top->addWidget(histBtn);
    left->addLayout(top);

    // дисплей (HUD)
    m_historyLine = new QLabel;
    m_historyLine->setObjectName("history");
    m_historyLine->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_historyLine->setMinimumHeight(24);
    left->addWidget(m_historyLine);

    m_exprEdit = new QLineEdit;
    m_exprEdit->setReadOnly(true);
    m_exprEdit->setAlignment(Qt::AlignRight);
    m_exprEdit->setFocusPolicy(Qt::NoFocus);
    m_exprEdit->setMinimumHeight(60);
    left->addWidget(m_exprEdit);

    m_previewLabel = new QLabel;
    m_previewLabel->setObjectName("preview");
    m_previewLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_previewLabel->setMinimumHeight(32);
    left->addWidget(m_previewLabel);

    // кнопки
    auto* grid = new QGridLayout;
    grid->setSpacing(6);
    left->addLayout(grid, 1);

    auto add = [&](const QString& text, const char* role, int r, int c, int rs, int cs,
                   std::function<void()> fn) -> QPushButton* {
        auto* b = new QPushButton(text);
        b->setProperty("role", role);
        b->setFocusPolicy(Qt::NoFocus);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        b->setMinimumHeight(46);
        connect(b, &QPushButton::clicked, this, [fn] { fn(); });
        grid->addWidget(b, r, c, rs, cs);
        return b;
    };

    const QString sqrtSym(sym::Sqrt);

    // --- научный блок (колонки 0..2)
    m_degBtn = add("Deg", "sci", 0, 0, 1, 1, [this] { toggleDeg(); });
    m_invBtn = add("Inv", "sci", 0, 1, 1, 1, [this] { toggleInv(); });
    m_modeBtn = add("Дроби", "sci", 0, 2, 1, 1, [this] { toggleMode(); });

    m_sinBtn = add("sin", "sci", 1, 0, 1, 1, [this] { insertText(m_inv ? "asin(" : "sin("); });
    m_cosBtn = add("cos", "sci", 1, 1, 1, 1, [this] { insertText(m_inv ? "acos(" : "cos("); });
    m_tanBtn = add("tan", "sci", 1, 2, 1, 1, [this] { insertText(m_inv ? "atan(" : "tan("); });

    m_lnBtn = add("ln", "sci", 2, 0, 1, 1, [this] { insertText(m_inv ? "e^(" : "ln("); });
    m_logBtn = add("log", "sci", 2, 1, 1, 1, [this] { insertText(m_inv ? "10^(" : "log("); });
    add(sqrtSym, "sci", 2, 2, 1, 1, [this] { insertText(QString(sym::Sqrt) + "("); });

    add(QString(sym::Pi), "sci", 3, 0, 1, 1, [this] { insertText(QString(sym::Pi)); });
    add("e", "sci", 3, 1, 1, 1, [this] { insertText("e"); });
    add("xʸ", "sci", 3, 2, 1, 1, [this] { insertPostfix("^"); });

    add("x²", "sci", 4, 0, 1, 1, [this] { insertPostfix("^2"); });
    add("x!", "sci", 4, 1, 1, 1, [this] { insertPostfix("!"); });
    add("1/x", "sci", 4, 2, 1, 1, [this] { reciprocal(); });

    add(QStringLiteral("a⁄b"), "sci", 5, 0, 1, 3, [this] { insertPostfix(QString(sym::Frac)); });

    // --- цифровой блок (колонки 3..6)
    add("AC", "clear", 0, 3, 1, 1, [this] { clearAll(); });
    add("⌫", "clear", 0, 4, 1, 1, [this] { backspace(); });
    add("(", "op", 0, 5, 1, 1, [this] { insertText("("); });
    add(")", "op", 0, 6, 1, 1, [this] { insertText(")"); });

    add("7", "num", 1, 3, 1, 1, [this] { insertText("7"); });
    add("8", "num", 1, 4, 1, 1, [this] { insertText("8"); });
    add("9", "num", 1, 5, 1, 1, [this] { insertText("9"); });
    add(QString(sym::Divide), "op", 1, 6, 1, 1, [this] { insertOperator(sym::Divide); });

    add("4", "num", 2, 3, 1, 1, [this] { insertText("4"); });
    add("5", "num", 2, 4, 1, 1, [this] { insertText("5"); });
    add("6", "num", 2, 5, 1, 1, [this] { insertText("6"); });
    add(QString(sym::Times), "op", 2, 6, 1, 1, [this] { insertOperator(sym::Times); });

    add("1", "num", 3, 3, 1, 1, [this] { insertText("1"); });
    add("2", "num", 3, 4, 1, 1, [this] { insertText("2"); });
    add("3", "num", 3, 5, 1, 1, [this] { insertText("3"); });
    add(QString(sym::Minus), "op", 3, 6, 1, 1, [this] { insertOperator(sym::Minus); });

    add("0", "num", 4, 3, 1, 1, [this] { insertText("0"); });
    add(".", "num", 4, 4, 1, 1, [this] { insertText("."); });
    add("%", "op", 4, 5, 1, 1, [this] { insertPostfix("%"); });
    add("+", "op", 4, 6, 1, 1, [this] { insertOperator('+'); });

    add("=", "eq", 5, 3, 1, 4, [this] { equals(); });

    for (int c = 0; c < 7; ++c) grid->setColumnStretch(c, 1);
    for (int r = 0; r < 6; ++r) grid->setRowStretch(r, 1);

    // --- панель истории
    m_histPanel = new QWidget;
    m_histPanel->setFixedWidth(240);
    auto* hv = new QVBoxLayout(m_histPanel);
    hv->setContentsMargins(0, 0, 0, 0);
    auto* title = new QLabel(QStringLiteral("История"));
    title->setStyleSheet("font-size:18px; font-weight:600;");
    m_histList = new QListWidget;
    m_histList->setWordWrap(true);
    m_histList->setFocusPolicy(Qt::NoFocus);
    connect(m_histList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        m_expr = it->data(Qt::UserRole).toString();
        m_justEvaluated = false;
        m_historyLine->clear();
        refresh();
    });
    auto* clearHist = new QPushButton(QStringLiteral("Очистить историю"));
    clearHist->setProperty("role", "flat");
    clearHist->setFocusPolicy(Qt::NoFocus);
    connect(clearHist, &QPushButton::clicked, m_histList, &QListWidget::clear);
    hv->addWidget(title);
    hv->addWidget(m_histList, 1);
    hv->addWidget(clearHist);
    m_histPanel->hide();
    root->addWidget(m_histPanel);
}

void MainWindow::setActive(QPushButton* b, bool on) {
    b->setProperty("active", on);
    b->style()->unpolish(b);
    b->style()->polish(b);
}

// ------------------------------------------------------------------ ввод
void MainWindow::insertText(const QString& s) {
    if (m_justEvaluated) {
        m_expr.clear();
        m_justEvaluated = false;
        m_historyLine->clear();
    }
    m_expr += s;
    refresh();
}

void MainWindow::insertOperator(QChar op) {
    if (m_justEvaluated) {
        m_justEvaluated = false;
        m_historyLine->clear();
    }
    if (m_expr.isEmpty()) {
        if (op == sym::Minus) { m_expr = op; refresh(); }
        return;
    }
    const QChar last = m_expr.back();
    if (isBinaryOp(last)) {
        const bool allowSign = (op == sym::Minus) && (last == sym::Times || last == sym::Divide);
        if (allowSign) m_expr += op;
        else m_expr.back() = op;
    } else if (last == '(' && op != sym::Minus) {
        return;
    } else {
        m_expr += op;
    }
    refresh();
}

bool MainWindow::canPostfix() const {
    if (m_expr.isEmpty()) return false;
    const QChar last = m_expr.back();
    return !isBinaryOp(last) && last != '(' && last != '^' && last != sym::Frac && last != sym::Sqrt
           && last != '.';
}

void MainWindow::insertPostfix(const QString& s) {
    if (!canPostfix()) return;
    if (m_justEvaluated) {
        m_justEvaluated = false;
        m_historyLine->clear();
    }
    m_expr += s;
    refresh();
}

void MainWindow::reciprocal() {
    if (m_expr.isEmpty()) return;
    m_justEvaluated = false;
    m_historyLine->clear();
    m_expr = QStringLiteral("1") + sym::Divide + QStringLiteral("(") + m_expr + QStringLiteral(")");
    refresh();
}

void MainWindow::backspace() {
    if (m_justEvaluated) {
        m_justEvaluated = false;
        m_historyLine->clear();
    }
    if (m_expr.isEmpty()) return;
    for (const QString& f : kFuncPrefixes) {
        if (m_expr.endsWith(f)) {
            m_expr.chop(f.size());
            refresh();
            return;
        }
    }
    m_expr.chop(1);
    refresh();
}

void MainWindow::clearAll() {
    m_expr.clear();
    m_justEvaluated = false;
    m_historyLine->clear();
    refresh();
}

void MainWindow::equals() {
    if (m_expr.isEmpty()) return;
    try {
        const QString expr = m_expr;
        const Value v = evaluate(expr, m_deg);
        showResult(v, expr);
    } catch (const CalcError& e) {
        showError(e.msg);
    }
}

// ------------------------------------------------------------------ отображение
void MainWindow::refresh() {
    m_exprEdit->setText(m_expr);
    if (!m_justEvaluated) updatePreview();
}

void MainWindow::updatePreview() {
    m_previewLabel->setStyleSheet(QString());
    m_previewLabel->clear();
    if (m_expr.isEmpty()) return;
    try {
        const Formatted f = format(evaluate(m_expr, m_deg), m_fractionMode);
        if (f.main == m_expr && f.extra.isEmpty()) return;   // просто число - превью не нужно
        m_previewLabel->setText(f.main + (f.extra.isEmpty() ? QString() : QStringLiteral("     ") + f.extra));
    } catch (const CalcError&) {
        // выражение ещё не закончено - молчим
    }
}

void MainWindow::showResult(const Value& v, const QString& exprForHistory) {
    const Formatted f = format(v, m_fractionMode);
    m_lastValue = v;
    m_hasLast = true;
    if (!exprForHistory.isEmpty()) {
        m_historyLine->setText(exprForHistory + QStringLiteral(" ="));
        addHistory(exprForHistory, f.main);
    }
    m_expr = f.cont;
    m_justEvaluated = true;
    m_exprEdit->setText(m_expr);
    m_previewLabel->setStyleSheet(QString());
    m_previewLabel->setText(f.extra);
}

void MainWindow::showError(const QString& msg) {
    m_previewLabel->setStyleSheet("color:#ff6b6b;");
    m_previewLabel->setText(msg);
}

void MainWindow::addHistory(const QString& expr, const QString& result) {
    auto* item = new QListWidgetItem(expr + QStringLiteral("\n= ") + result);
    item->setData(Qt::UserRole, expr);
    item->setTextAlignment(Qt::AlignRight);
    m_histList->insertItem(0, item);
}

// ------------------------------------------------------------------ режимы
void MainWindow::toggleDeg() {
    m_deg = !m_deg;
    m_degBtn->setText(m_deg ? "Deg" : "Rad");
    if (!m_justEvaluated) updatePreview();
}

void MainWindow::toggleInv() {
    m_inv = !m_inv;
    setActive(m_invBtn, m_inv);
    m_sinBtn->setText(m_inv ? "sin⁻¹" : "sin");
    m_cosBtn->setText(m_inv ? "cos⁻¹" : "cos");
    m_tanBtn->setText(m_inv ? "tan⁻¹" : "tan");
    m_lnBtn->setText(m_inv ? "eˣ" : "ln");
    m_logBtn->setText(m_inv ? "10ˣ" : "log");
}

void MainWindow::toggleMode() {
    m_fractionMode = !m_fractionMode;
    m_modeBtn->setText(m_fractionMode ? "Дроби" : "Десят.");
    if (m_justEvaluated && m_hasLast) showResult(m_lastValue, QString());
    else updatePreview();
}

void MainWindow::toggleHistory() {
    const bool show = !m_histPanel->isVisible();
    m_histPanel->setVisible(show);
    resize(width() + (show ? 252 : -252), height());
}

// ------------------------------------------------------------------ клавиатура
void MainWindow::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:     equals(); return;
    case Qt::Key_Backspace: backspace(); return;
    case Qt::Key_Escape:
    case Qt::Key_Delete:    clearAll(); return;
    default: break;
    }

    const QString t = e->text();
    if (t.size() != 1) { QWidget::keyPressEvent(e); return; }
    const QChar c = t[0];

    if (c.isDigit())                insertText(t);
    else if (c == '.' || c == ',')  insertText(".");
    else if (c == '+')              insertOperator('+');
    else if (c == '-')              insertOperator(sym::Minus);
    else if (c == '*')              insertOperator(sym::Times);
    else if (c == '/')              insertOperator(sym::Divide);
    else if (c == '(' || c == ')')  insertText(t);
    else if (c == '^')              insertPostfix("^");
    else if (c == '!')              insertPostfix("!");
    else if (c == '%')              insertPostfix("%");
    else if (c == '\\')             insertPostfix(QString(sym::Frac));   // клавиша "\" = дробная черта
    else if (c == 'p' || c == 'P')  insertText(QString(sym::Pi));
    else if (c == 'e')              insertText("e");
    else QWidget::keyPressEvent(e);
}
