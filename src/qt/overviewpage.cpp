// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "freedomsend.h"
#include "freedomsendconfig.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "init.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>

#define DECORATION_SIZE 48
#define ICON_OFFSET 16
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::PatriotBit)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));


    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelFreedomsendSyncStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    showingFreedomSendMessage = 0;
    freedomsendActionCheck = 0;
    lastNewBlock = 0;

    if(fLiteMode){
        ui->frameFreedomsend->setVisible(false);
    } else if(!fMasterNode) {
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(freedomSendStatus()));
        timer->start(333);
    }

    if(fMasterNode){
        ui->toggleFreedomsend->setText("(" + tr("Disabled") + ")");
        ui->freedomsendAuto->setText("(" + tr("Disabled") + ")");
        ui->freedomsendReset->setText("(" + tr("Disabled") + ")");
        ui->frameFreedomsend->setEnabled(false);
    }else if(!fEnableFreedomsend){
        ui->toggleFreedomsend->setText(tr("Go for FreedomSend"));
    } else {
        ui->toggleFreedomsend->setText(tr("Cease FreedomSend"));
    }

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    if(!fLiteMode && !fMasterNode) disconnect(timer, SIGNAL(timeout()), this, SLOT(freedomSendStatus()));
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 anonymizedBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));
    ui->labelAnonymized->setText(BitcoinUnits::formatWithUnit(unit, anonymizedBalance));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);

    if(cachedTxLocks != nCompleteTXLocks){
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(ui->freedomsendAuto, SIGNAL(clicked()), this, SLOT(freedomsendAuto()));
        connect(ui->freedomsendReset, SIGNAL(clicked()), this, SLOT(freedomsendReset()));
        connect(ui->toggleFreedomsend, SIGNAL(clicked()), this, SLOT(toggleFreedomsend()));
    }

    // update the display unit, to not use the default ("PatriotBit")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelFreedomsendSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateFreedomsendProgress()
{
    if(IsInitialBlockDownload()) return;

    int64_t nBalance = pwalletMain->GetBalance();
    if(nBalance == 0)
    {
        ui->freedomsendProgress->setValue(0);
        QString s(tr("No inputs detected"));
        ui->freedomsendProgress->setToolTip(s);
        return;
    }

    //get denominated unconfirmed inputs
    if(pwalletMain->GetDenominatedBalance(true, true) > 0)
    {
        QString s(tr("Found unconfirmed denominated outputs, will wait till they confirm to recalculate."));
        ui->freedomsendProgress->setToolTip(s);
        return;
    }

    //Get the anon threshold
    int64_t nMaxToAnonymize = nAnonymizePatriotBitAmount*COIN;

    // If it's more than the wallet amount, limit to that.
    if(nMaxToAnonymize > nBalance) nMaxToAnonymize = nBalance;

    if(nMaxToAnonymize == 0) return;

    // calculate parts of the progress, each of them shouldn't be higher than 1:
    // mixing progress of denominated balance
    int64_t denominatedBalance = pwalletMain->GetDenominatedBalance();
    float denomPart = 0;
    if(denominatedBalance > 0)
    {
        denomPart = (float)pwalletMain->GetNormalizedAnonymizedBalance() / denominatedBalance;
        denomPart = denomPart > 1 ? 1 : denomPart;
        if(denomPart == 1 && nMaxToAnonymize > denominatedBalance) nMaxToAnonymize = denominatedBalance;
    }

    // % of fully anonymized balance
    float anonPart = 0;
    if(nMaxToAnonymize > 0)
    {
        anonPart = (float)pwalletMain->GetAnonymizedBalance() / nMaxToAnonymize;
        // if anonPart is > 1 then we are done, make denomPart equal 1 too
        anonPart = anonPart > 1 ? (denomPart = 1, 1) : anonPart;
    }

    // apply some weights to them (sum should be <=100) and calculate the whole progress
    int progress = 80 * denomPart + 20 * anonPart;
    if(progress >= 100) progress = 100;

    ui->freedomsendProgress->setValue(progress);

    std::ostringstream convert;
    convert << "Progress: " << progress << "%, inputs have an average of " << pwalletMain->GetAverageAnonymizedRounds() << " of " << nFreedomsendRounds << " rounds";
    QString s(convert.str().c_str());
    ui->freedomsendProgress->setToolTip(s);
}


void OverviewPage::freedomSendStatus()
{
    int nBestHeight = chainActive.Tip()->nHeight;

    if(nBestHeight != freedomSendPool.cachedNumBlocks)
    {
        //we we're processing lots of blocks, we'll just leave
        if(GetTime() - lastNewBlock < 10) return;
        lastNewBlock = GetTime();

        updateFreedomsendProgress();

        QString strSettings(" " + tr("Rounds"));
        strSettings.prepend(QString::number(nFreedomsendRounds)).prepend(" / ");
        strSettings.prepend(BitcoinUnits::formatWithUnit(
            walletModel->getOptionsModel()->getDisplayUnit(),
            nAnonymizePatriotBitAmount * COIN)
        );

        ui->labelAmountRounds->setText(strSettings);
    }

    if(!fEnableFreedomsend) {
        if(nBestHeight != freedomSendPool.cachedNumBlocks)
        {
            freedomSendPool.cachedNumBlocks = nBestHeight;

            ui->freedomsendEnabled->setText(tr("Disabled"));
            ui->freedomsendStatus->setText("");
            ui->toggleFreedomsend->setText(tr("Begin FreedomSend"));
        }

        return;
    }

    // check freedomsend status and unlock if needed
    if(nBestHeight != freedomSendPool.cachedNumBlocks)
    {
        // Balance and number of transactions might have changed
        freedomSendPool.cachedNumBlocks = nBestHeight;

        /* *******************************************************/

        ui->freedomsendEnabled->setText(tr("Enabled"));
    }

    int state = freedomSendPool.GetState();
    int entries = freedomSendPool.GetEntriesCount();
    int accepted = freedomSendPool.GetLastEntryAccepted();

    /* ** @TODO this string creation really needs some clean ups ---vertoe ** */
    std::ostringstream convert;

    if(state == POOL_STATUS_IDLE) {
        convert << tr("Freedomsend is idle.").toStdString();
    } else if(state == POOL_STATUS_ACCEPTING_ENTRIES) {
        if(entries == 0) {
            if(freedomSendPool.strAutoDenomResult.size() == 0){
                convert << tr("Mixing in progress...").toStdString();
            } else {
                convert << freedomSendPool.strAutoDenomResult;
            }
            showingFreedomSendMessage = 0;
        } else if (accepted == 1) {
            convert << tr("Freedomsend request complete: Your transaction was accepted into the pool!").toStdString();
            if(showingFreedomSendMessage % 10 > 8) {
                freedomSendPool.lastEntryAccepted = 0;
                showingFreedomSendMessage = 0;
            }
        } else {
            if(showingFreedomSendMessage % 70 <= 40) convert << tr("Submitted following entries to masternode:").toStdString() << " " << entries << "/" << freedomSendPool.GetMaxPoolTransactions();
            else if(showingFreedomSendMessage % 70 <= 50) convert << tr("Submitted to masternode, waiting for more entries").toStdString() << " (" << entries << "/" << freedomSendPool.GetMaxPoolTransactions() << " ) .";
            else if(showingFreedomSendMessage % 70 <= 60) convert << tr("Submitted to masternode, waiting for more entries").toStdString() << " (" << entries << "/" << freedomSendPool.GetMaxPoolTransactions() << " ) ..";
            else if(showingFreedomSendMessage % 70 <= 70) convert << tr("Submitted to masternode, waiting for more entries").toStdString() << " (" << entries << "/" << freedomSendPool.GetMaxPoolTransactions() << " ) ...";
        }
    } else if(state == POOL_STATUS_SIGNING) {
        if(showingFreedomSendMessage % 70 <= 10) convert << tr("Found enough users, signing ...").toStdString();
        else if(showingFreedomSendMessage % 70 <= 20) convert << tr("Found enough users, signing ( waiting").toStdString() << ". )";
        else if(showingFreedomSendMessage % 70 <= 30) convert << tr("Found enough users, signing ( waiting").toStdString() << ".. )";
        else if(showingFreedomSendMessage % 70 <= 40) convert << tr("Found enough users, signing ( waiting").toStdString() << "... )";
    } else if(state == POOL_STATUS_TRANSMISSION) {
        convert << tr("Transmitting final transaction.").toStdString();
    } else if (state == POOL_STATUS_IDLE) {
        convert << tr("Freedomsend is idle.").toStdString();
    } else if (state == POOL_STATUS_FINALIZE_TRANSACTION) {
        convert << tr("Finalizing transaction.").toStdString();
    } else if(state == POOL_STATUS_ERROR) {
        convert << tr("Freedomsend request incomplete:").toStdString() << " " << freedomSendPool.lastMessage << ". " << tr("Will retry...").toStdString();
    } else if(state == POOL_STATUS_SUCCESS) {
        convert << tr("Freedomsend request complete:").toStdString() << " " << freedomSendPool.lastMessage;
    } else if(state == POOL_STATUS_QUEUE) {
        if(showingFreedomSendMessage % 70 <= 50) convert << tr("Submitted to masternode, waiting in queue").toStdString() << " .";
        else if(showingFreedomSendMessage % 70 <= 60) convert << tr("Submitted to masternode, waiting in queue").toStdString() << " ..";
        else if(showingFreedomSendMessage % 70 <= 70) convert << tr("Submitted to masternode, waiting in queue").toStdString() << " ...";
    } else {
        convert << tr("Unknown state:").toStdString() << " id = " << state;
    }

    if(state == POOL_STATUS_ERROR || state == POOL_STATUS_SUCCESS) freedomSendPool.Check();

    QString s(convert.str().c_str());
    s = tr("Last Freedomsend message:\n") + s;

    if(s != ui->freedomsendStatus->text())
        LogPrintf("Last Freedomsend message: %s\n", convert.str().c_str());

    ui->freedomsendStatus->setText(s);

    if(freedomSendPool.sessionDenom == 0){
        ui->labelSubmittedDenom->setText(tr("N/A"));
    } else {
        std::string out;
        freedomSendPool.GetDenominationsToString(freedomSendPool.sessionDenom, out);
        QString s2(out.c_str());
        ui->labelSubmittedDenom->setText(s2);
    }

    showingFreedomSendMessage++;
    freedomsendActionCheck++;

    // Get FreedomSend Denomination Status
}

void OverviewPage::freedomsendAuto(){
    freedomSendPool.DoAutomaticDenominating();
}

void OverviewPage::freedomsendReset(){
    freedomSendPool.Reset();

    QMessageBox::warning(this, tr("Freedomsend"),
        tr("Freedomsend was successfully reset."),
        QMessageBox::Ok, QMessageBox::Ok);
}

void OverviewPage::toggleFreedomsend(){
    if(!fEnableFreedomsend){
        int64_t balance = pwalletMain->GetBalance();
        float minAmount = 1.49 * COIN;
        if(balance < minAmount){
            QString strMinAmount(
                BitcoinUnits::formatWithUnit(
                    walletModel->getOptionsModel()->getDisplayUnit(),
                    minAmount));
            QMessageBox::warning(this, tr("Freedomsend"),
                tr("Freedomsend requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // if wallet is locked, ask for a passphrase
        if (walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock(false));
            if(!ctx.isValid())
            {
                //unlock was cancelled
                freedomSendPool.cachedNumBlocks = 0;
                QMessageBox::warning(this, tr("Freedomsend"),
                    tr("Wallet is locked and user declined to unlock. Disabling Freedomsend."),
                    QMessageBox::Ok, QMessageBox::Ok);
                if (fDebug) LogPrintf("Wallet is locked and user declined to unlock. Disabling Freedomsend.\n");
                return;
            }
        }

    }

    freedomSendPool.cachedNumBlocks = 0;
    fEnableFreedomsend = !fEnableFreedomsend;

    if(!fEnableFreedomsend){
        ui->toggleFreedomsend->setText(tr("BeginFreedomSend"));
    } else {
        ui->toggleFreedomsend->setText(tr("End FreedomSend"));

        /* show freedomsend configuration if client has defaults set */

        if(nAnonymizePatriotBitAmount == 0){
            FreedomsendConfig dlg(this);
            dlg.setModel(walletModel);
            dlg.exec();
        }

        freedomSendPool.DoAutomaticDenominating();
    }
}
