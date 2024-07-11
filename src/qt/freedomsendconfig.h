#ifndef FREEDOMSENDCONFIG_H
#define FREEDOMSENDCONFIG_H

#include <QDialog>

namespace Ui {
    class FreedomsendConfig;
}
class WalletModel;

/** Multifunctional dialog to ask for passphrases. Used for encryption, unlocking, and changing the passphrase.
 */
class FreedomsendConfig : public QDialog
{
    Q_OBJECT

public:

    FreedomsendConfig(QWidget *parent = 0);
    ~FreedomsendConfig();

    void setModel(WalletModel *model);


private:
    Ui::FreedomsendConfig *ui;
    WalletModel *model;
    void configure(bool enabled, int coins, int rounds);

private slots:

    void clickBasic();
    void clickHigh();
    void clickMax();
};

#endif // FREEDOMSENDCONFIG_H
