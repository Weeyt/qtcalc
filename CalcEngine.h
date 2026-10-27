#pragma once

#include <QString>
#include <QtNumeric>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace calc {

struct CalcError { QString msg; };

namespace sym {
inline const QChar Minus(0x2212);   // −
inline const QChar Times(0x00D7);   // ×
inline const QChar Divide(0x00F7);  // ÷
inline const QChar Sqrt(0x221A);    // √
inline const QChar Pi(0x03C0);      // π
inline const QChar Frac(0x2044);    // ⁄
}

constexpr double kPi = 3.14159265358979323846;

[[noreturn]] inline void fail(const QString& m = QStringLiteral("Error")) { throw CalcError{m}; }

struct Value {
    bool exact = true;   // true: n/d, false: x (double)
    qint64 n = 0;
    qint64 d = 1;
    double x = 0.0;

    static Value integer(qint64 v) { Value r; r.n = v; return r; }
    static Value real(double v);
    static Value fraction(qint64 n, qint64 d);

    double toDouble() const { return exact ? double(n) / double(d) : x; }
    bool isInteger() const { return exact && d == 1; }
    bool isZero() const { return exact ? n == 0 : x == 0.0; }
};

namespace detail {
struct Overflow {};

inline qint64 addI(qint64 a, qint64 b) { qint64 r; if (qAddOverflow(a, b, &r)) throw Overflow{}; return r; }
inline qint64 subI(qint64 a, qint64 b) { qint64 r; if (qSubOverflow(a, b, &r)) throw Overflow{}; return r; }
inline qint64 mulI(qint64 a, qint64 b) { qint64 r; if (qMulOverflow(a, b, &r)) throw Overflow{}; return r; }

inline Value make(qint64 n, qint64 d) {
    if (d == 0) fail(QStringLiteral("Division by 0"));
    constexpr qint64 kMin = std::numeric_limits<qint64>::min();
    if (n == kMin || d == kMin) throw Overflow{};
    if (d < 0) { n = -n; d = -d; }
    const qint64 g = std::gcd(n, d);
    if (g > 1) { n /= g; d /= g; }
    Value v;
    v.n = n;
    v.d = d;
    return v;
}
} // namespace detail

inline Value Value::real(double v) {
    if (!std::isfinite(v)) fail();
    const double r = std::round(v);
    const double tol = (r == 0.0) ? 1e-14 : 1e-12 * std::fabs(v);
    if (std::fabs(v - r) <= tol && std::fabs(r) < 9e15)
        return integer(static_cast<qint64>(r));
    Value out;
    out.exact = false;
    out.x = v;
    return out;
}

inline Value Value::fraction(qint64 n, qint64 d) {
    try { return detail::make(n, d); }
    catch (const detail::Overflow&) { return real(double(n) / double(d)); }
}

inline Value add(const Value& a, const Value& b) {
    if (a.exact && b.exact) {
        try {
            using namespace detail;
            return make(addI(mulI(a.n, b.d), mulI(b.n, a.d)), mulI(a.d, b.d));
        } catch (const detail::Overflow&) {}
    }
    return Value::real(a.toDouble() + b.toDouble());
}

inline Value sub(const Value& a, const Value& b) {
    if (a.exact && b.exact) {
        try {
            using namespace detail;
            return make(subI(mulI(a.n, b.d), mulI(b.n, a.d)), mulI(a.d, b.d));
        } catch (const detail::Overflow&) {}
    }
    return Value::real(a.toDouble() - b.toDouble());
}

inline Value mul(const Value& a, const Value& b) {
    if (a.exact && b.exact) {
        try {
            using namespace detail;
            const qint64 g1 = std::gcd(a.n, b.d);   // b.d >= 1 -> g1 >= 1
            const qint64 g2 = std::gcd(b.n, a.d);
            return make(mulI(a.n / g1, b.n / g2), mulI(a.d / g2, b.d / g1));
        } catch (const detail::Overflow&) {}
    }
    return Value::real(a.toDouble() * b.toDouble());
}

inline Value divide(const Value& a, const Value& b) {
    if (b.isZero()) fail(QStringLiteral("Деление на ноль"));
    if (a.exact && b.exact) {
        try {
            using namespace detail;
            const qint64 g1 = std::gcd(a.n, b.n);
            const qint64 g2 = std::gcd(a.d, b.d);
            return make(mulI(a.n / g1, b.d / g2), mulI(a.d / g2, b.n / g1));
        } catch (const detail::Overflow&) {}
    }
    return Value::real(a.toDouble() / b.toDouble());
}

inline Value negate(const Value& a) { return sub(Value::integer(0), a); }

inline Value power(const Value& a, const Value& b) {
    if (a.exact && b.isInteger() && b.n >= -1000 && b.n <= 1000) {
        qint64 e = b.n < 0 ? -b.n : b.n;
        Value result = Value::integer(1);
        Value base = a;
        while (e > 0) {
            if (e & 1) result = mul(result, base);
            e >>= 1;
            if (e) base = mul(base, base);
        }
        return b.n < 0 ? divide(Value::integer(1), result) : result;
    }
    const double x = a.toDouble();
    const double y = b.toDouble();
    if (x < 0 && y != std::floor(y)) fail();
    return Value::real(std::pow(x, y));
}

inline bool isqrt(qint64 v, qint64& out) {
    if (v < 0) return false;
    const qint64 r = static_cast<qint64>(std::llround(std::sqrt(double(v))));
    for (qint64 c = (r > 0 ? r - 1 : 0); c <= r + 1; ++c) {
        if (c > 3037000499LL) break;
        if (c * c == v) { out = c; return true; }
    }
    return false;
}

inline Value sqrtV(const Value& a) {
    const double x = a.toDouble();
    if (x < 0) fail();
    if (a.exact) {
        qint64 rn, rd;
        if (isqrt(a.n, rn) && isqrt(a.d, rd)) return Value::fraction(rn, rd);
    }
    return Value::real(std::sqrt(x));
}

inline Value factorial(const Value& a) {
    if (!a.isInteger() || a.n < 0 || a.n > 170) fail();
    if (a.n <= 20) {
        qint64 r = 1;
        for (qint64 i = 2; i <= a.n; ++i) r *= i;
        return Value::integer(r);
    }
    return Value::real(std::tgamma(double(a.n) + 1.0));
}

inline Value applyFunction(const QString& f, const Value& a, bool deg) {
    const double x = a.toDouble();
    if (f == "sin" || f == "cos" || f == "tan") {
        if (f == "tan" && deg && std::fmod(std::fabs(x), 180.0) == 90.0) fail();
        const double r = deg ? x * kPi / 180.0 : x;
        return Value::real(f == "sin" ? std::sin(r) : f == "cos" ? std::cos(r) : std::tan(r));
    }
    if (f == "asin" || f == "acos") {
        if (std::fabs(x) > 1.0) fail();
        double r = (f == "asin") ? std::asin(x) : std::acos(x);
        if (deg) r = r * 180.0 / kPi;
        return Value::real(r);
    }
    if (f == "atan") {
        double r = std::atan(x);
        if (deg) r = r * 180.0 / kPi;
        return Value::real(r);
    }
    if (f == "ln")  { if (x <= 0) fail(); return Value::real(std::log(x)); }
    if (f == "log") { if (x <= 0) fail(); return Value::real(std::log10(x)); }
    fail();
}

class Parser {
public:
    Parser(const QString& s, bool deg) : m_s(s), m_deg(deg) {}

    Value parse() {
        if (m_s.trimmed().isEmpty()) fail();
        const Value v = expr();
        if (m_i < m_s.size()) fail();
        return v;
    }

private:
    QString m_s;
    int m_i = 0;
    bool m_deg;

    QChar peek() const { return m_i < m_s.size() ? m_s[m_i] : QChar(); }
    static bool isDigit(QChar c) { return c >= QChar('0') && c <= QChar('9'); }

    QString matchFunction() const {
        static const char* names[] = {"asin", "acos", "atan", "sin", "cos", "tan", "ln", "log"};
        for (const char* nm : names) {
            const QString name = QString::fromLatin1(nm);
            const int len = name.size();
            if (m_s.mid(m_i, len) == name && m_i + len < m_s.size() && m_s[m_i + len] == QChar('('))
                return name;
        }
        return {};
    }

    bool startsPrimary() const {
        const QChar c = peek();
        if (isDigit(c) || c == '.' || c == ',' || c == '(' || c == sym::Sqrt || c == sym::Pi || c == 'e')
            return true;
        return !matchFunction().isEmpty();
    }

    void closeParen() {
        if (peek() == ')') ++m_i;
        else if (m_i < m_s.size()) fail();
    }

    Value expr() {
        Value v = term();
        for (;;) {
            const QChar c = peek();
            if (c == '+') { ++m_i; v = add(v, term()); }
            else if (c == '-' || c == sym::Minus) { ++m_i; v = sub(v, term()); }
            else break;
        }
        return v;
    }

    Value term() {
        Value v = unary();
        for (;;) {
            const QChar c = peek();
            if (c == sym::Times || c == '*') { ++m_i; v = mul(v, unary()); }
            else if (c == sym::Divide || c == '/') { ++m_i; v = divide(v, unary()); }
            else if (startsPrimary()) { v = mul(v, unary()); }   // 2π, 3(4+1), 2√9 ...
            else break;
        }
        return v;
    }

    Value unary() {
        const QChar c = peek();
        if (c == '-' || c == sym::Minus) { ++m_i; return negate(unary()); }
        if (c == '+') { ++m_i; return unary(); }
        return power();
    }

    Value power() {
        const Value b = postfix();
        if (peek() == '^') { ++m_i; return calc::power(b, unary()); }
        return b;
    }

    Value postfix() {
        Value v = frac();
        for (;;) {
            const QChar c = peek();
            if (c == '!') { ++m_i; v = factorial(v); }
            else if (c == '%') { ++m_i; v = divide(v, Value::integer(100)); }
            else break;
        }
        return v;
    }

    Value frac() {
        Value v = primary();
        while (peek() == sym::Frac) {
            ++m_i;
            v = divide(v, primary());
        }
        return v;
    }

    Value primary() {
        const QChar c = peek();
        if (isDigit(c) || c == '.' || c == ',') return number();
        if (c == '(') { ++m_i; const Value v = expr(); closeParen(); return v; }
        if (c == sym::Pi) { ++m_i; return Value::real(kPi); }
        if (c == 'e') { ++m_i; return Value::real(std::exp(1.0)); }
        if (c == sym::Sqrt) { ++m_i; return sqrtV(frac()); }
        const QString fn = matchFunction();
        if (!fn.isEmpty()) {
            m_i += fn.size() + 1;
            const Value arg = expr();
            closeParen();
            return applyFunction(fn, arg, m_deg);
        }
        fail();
    }

    Value number() {
        QString ip, fp;
        while (isDigit(peek())) ip += m_s[m_i++];
        if (peek() == '.' || peek() == ',') {
            ++m_i;
            while (isDigit(peek())) fp += m_s[m_i++];
        }
        if (ip.isEmpty() && fp.isEmpty()) fail();

        Value v;
        const QString digits = ip + fp;
        if (digits.size() <= 18) {
            qint64 pow10 = 1;
            for (int k = 0; k < fp.size(); ++k) pow10 *= 10;
            v = Value::fraction(digits.toLongLong(), pow10);
        } else {
            v = Value::real((ip + QLatin1Char('.') + fp).toDouble());
        }

        if (peek() == 'E') {
            const int save = m_i;
            ++m_i;
            int sign = 1;
            if (peek() == '+') ++m_i;
            else if (peek() == '-' || peek() == sym::Minus) { sign = -1; ++m_i; }
            QString ed;
            while (isDigit(peek())) ed += m_s[m_i++];
            if (ed.isEmpty() || ed.size() > 4) { m_i = save; }
            else v = Value::real(v.toDouble() * std::pow(10.0, sign * ed.toInt()));
        }
        return v;
    }
};

inline Value evaluate(const QString& expression, bool deg) {
    Parser p(expression, deg);
    return p.parse();
}

struct Formatted {
    QString main;   // MAIN result
    QString extra;  // additional result
    QString cont;
};

inline QString formatDecimal(double x) {
    if (x == 0.0) x = 0.0;
    QString s = QString::number(x, 'g', 12);
    s.replace(QStringLiteral("e+"), QStringLiteral("E"));
    s.replace(QStringLiteral("e-"), QStringLiteral("E-"));
    return s;
}

inline bool isTerminating(qint64 d) {
    while (d % 2 == 0) d /= 2;
    while (d % 5 == 0) d /= 5;
    return d == 1;
}

inline Formatted format(const Value& v, bool fractionMode) {
    Formatted f;
    if (v.exact && v.d == 1) { f.main = QString::number(v.n); f.cont = f.main; return f; }
    if (!v.exact)            { f.main = formatDecimal(v.x);   f.cont = f.main; return f; }

    const QString frac = QString::number(v.n) + sym::Frac + QString::number(v.d);
    const double dv = double(v.n) / double(v.d);
    const QString dec = formatDecimal(dv);
    const QString decText = (isTerminating(v.d) ? QStringLiteral("= ") : QStringLiteral("≈ ")) + dec;

    QString mixed;
    const qint64 absN = v.n < 0 ? -v.n : v.n;
    if (absN > v.d)
        mixed = QString::number(v.n / v.d) + QLatin1Char(' ')
              + QString::number((v.n < 0 ? -(v.n % v.d) : (v.n % v.d))) + sym::Frac + QString::number(v.d);

    if (fractionMode) {
        f.main = frac;
        f.extra = mixed + (mixed.isEmpty() ? QString() : QStringLiteral("   ")) + decText;
        f.cont = frac;
    } else {
        f.main = dec;
        f.extra = (mixed.isEmpty() ? frac : mixed + QStringLiteral("   (") + frac + QLatin1Char(')'));
        f.cont = dec;
    }
    return f;
}

}
