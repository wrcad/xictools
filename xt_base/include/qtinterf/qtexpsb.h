#include <QDoubleSpinBox>
#include "spnumber/spnumber.h"


//----------------------------------------------------------------------------
// A Double Spin Box that uses exponential notation.
//

class ExpDoubleSpinbox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit ExpDoubleSpinbox(QWidget *parent = 0) : QDoubleSpinBox(parent) {}

    double valueFromText(const QString & text) const
    {
        QByteArray text_ba = text.toLatin1();
        const char *str = text_ba.constData();
        double *d = SPnum.parse(&str, true);
        if (d)
            return (*d);
        return (0.0/0.0);  // NaN, "can't happen"
    }

    QString textFromValue(double value) const
    {
        const char *str = SPnum.printnum(value, 0, true, decimals());
        return (QString(str));
    }

    // Change the way we validate user input (if validate => valueFromText)
    QValidator::State validate(QString &text, int&) const
    {
        QByteArray text_ba = text.toLatin1();
        const char *str = text_ba.constData();
        double *d = SPnum.parse(&str, true);
        return (d ? QValidator::Acceptable : QValidator::Invalid);
    }
};

