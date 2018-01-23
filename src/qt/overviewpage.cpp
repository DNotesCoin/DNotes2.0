#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "clientmodel.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "webutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>

#include "iomanip"

#define DECORATION_SIZE 64
#define NUM_ITEMS 6

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }

        painter->setPen(fUseBlackTheme ? QColor(255, 255, 255) : foreground);
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
        painter->setPen(fUseBlackTheme ? QColor(255, 255, 255) : foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(fUseBlackTheme ? QColor(96, 101, 110) : option.palette.color(QPalette::Text));
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
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    lastPriceRequested = 0;
    lastNewsRequested = 0;
    currentPrice = 0.0;
    dnotesNews = "";
    dceBriefNews = "";

    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    if (fUseBlackTheme)
    {
        const char* whiteLabelQSS = "QLabel { color: rgb(255,255,255); }";
        ui->labelBalance->setStyleSheet(whiteLabelQSS);
        ui->labelStake->setStyleSheet(whiteLabelQSS);
        ui->labelUnconfirmed->setStyleSheet(whiteLabelQSS);
        ui->labelImmature->setStyleSheet(whiteLabelQSS);
        ui->labelTotal->setStyleSheet(whiteLabelQSS);
    }
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + stake + unconfirmedBalance + immatureBalance));
    ui->labelFiat->setText(getFiatLabel(balance + stake + unconfirmedBalance + immatureBalance));

    updateNewsFeeds();

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);
}

struct XmlParsingInfo
{
    std::string xmlString;
    int startingIndex;
};

std::string getNextTitle(XmlParsingInfo &xmlInfo)
{
    std::string openingElement = "<title>";
    std::string closingElement = "</title>";
    std::string::size_type searchStringStartPosition = xmlInfo.xmlString.find(openingElement, xmlInfo.startingIndex);
    if(searchStringStartPosition != std::string::npos)
    {
        int titleStartPosition = searchStringStartPosition + openingElement.length();
        int closingStartPosition = xmlInfo.xmlString.find(closingElement, titleStartPosition + 1);

        std::string title = xmlInfo.xmlString.substr(titleStartPosition, closingStartPosition - titleStartPosition);
        xmlInfo.startingIndex = closingStartPosition + closingElement.length() + 1;
        return title;
    }

    return "";
}

std::string getNextLink(XmlParsingInfo &xmlInfo)
{
    std::string openingElement = "<link>";
    std::string closingElement = "</link>";
    std::string::size_type searchStringStartPosition = xmlInfo.xmlString.find(openingElement, xmlInfo.startingIndex);
    if(searchStringStartPosition != std::string::npos)
    {
        int linkStartPosition = searchStringStartPosition + openingElement.length();
        int closingStartPosition = xmlInfo.xmlString.find(closingElement, linkStartPosition + 1);

        std::string link = xmlInfo.xmlString.substr(linkStartPosition, closingStartPosition - linkStartPosition);
        xmlInfo.startingIndex = closingStartPosition + closingElement.length() + 1;
        return link;
    }   
    return "";
}

QString getFeedListItem(std::string title, std::string link)
{
    if(title != "" && link != "")
    {
        return "<li><a href=\"" + QString::fromStdString(link) + "\">" + QString::fromStdString(title) + "</a></li>";
    }
    return "";
}

void OverviewPage::updateNewsFeeds()
{
    time_t now = time(0);
    if(lastNewsRequested < now - 600)
    {
        lastPriceRequested = now;

        //DNotesCoin.com feed
        std::string dnotesResponse = WebUtil::getHttpResponseFromUrl("dnotescoin.com", "/feed/");

        if(dnotesResponse == "")
        {
            ui->browserDNotesNews->setVisible(false);
        }

        XmlParsingInfo xmlInfo;
        xmlInfo.xmlString = dnotesResponse;
        xmlInfo.startingIndex = 0;

        //first title/link are for the feed.
        std::string title = getNextTitle(xmlInfo);
        std::string link = getNextLink(xmlInfo);

        //get top 3 stories from feed
        dnotesNews = "<ul style='-qt-list-indent:1;'>";

        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);
        dnotesNews += getFeedListItem(title, link);
    
        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);
        dnotesNews += getFeedListItem(title, link);

        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);
        dnotesNews += getFeedListItem(title, link);
        
        dnotesNews += "</ul>";
        ui->browserDNotesNews->setHtml(dnotesNews);

        //DCEBrief feed
        std::string dceBriefResponse = WebUtil::getHttpsResponseFromUrl("dcebrief.com", "/feed/");
        
        if(dceBriefResponse == "")
        {
            ui->browserDNotesNews->setVisible(false);
        }

        xmlInfo.xmlString = dceBriefResponse;
        xmlInfo.startingIndex = 0;
        dceBriefNews = "<ul style='-qt-list-indent:1;'>";
        
        //first title/link are for the feed.
        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);

        //get top 3 stories from feed
        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);
        dceBriefNews += getFeedListItem(title, link);
    
        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);
        dceBriefNews += getFeedListItem(title, link);

        title = getNextTitle(xmlInfo);
        link = getNextLink(xmlInfo);
        dceBriefNews += getFeedListItem(title, link);
        
        dceBriefNews += "</ul>";
        ui->browserDCENews->setHtml(dceBriefNews);
    }
}

QString OverviewPage::getFiatLabel(qint64 totalCoins)
{
    time_t now = time(0);
    //cache price api request for 10 minutes
    if(lastPriceRequested < now - 600)
    {
        std::string apiResponse = WebUtil::getHttpsResponseFromUrl("api.coinmarketcap.com", "/v1/ticker/DNotes/?convert=USD");

        std::string searchString = "\"price_usd\": \"";
        std::string::size_type searchStringStartPosition = apiResponse.find(searchString);
        if(searchStringStartPosition != std::string::npos)
        {
            int priceStartPosition = searchStringStartPosition + searchString.length();
            int priceEndPosition = apiResponse.find("\"", priceStartPosition + 1);
            std::string priceString = apiResponse.substr(priceStartPosition, priceEndPosition - priceStartPosition);
            currentPrice = atof (priceString.c_str());
        }
        else
        {
            //hide fiat label if we can't reach the api or if the response is malformatted.
            ui->labelFiat->setVisible(false);
            return "";
        }

        lastPriceRequested = now;
    }

    double totalFiatValue = currentPrice * (totalCoins) / 100000000; //total coins is in satoshis

    //round to 2 decimal places
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << totalFiatValue;
    std::string roundedFiatValue = ss.str();


    return  "~$" + QString::fromStdString(roundedFiatValue) + " USD\r\n"
            "(1 NOTE \u2245 " + QString::number(currentPrice) + " USD)" ; //\u2245 is the approximately equal symbol in unicode.
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
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, walletModel->getStake(), currentUnconfirmedBalance, currentImmatureBalance);

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
    ui->labelTransactionsStatus->setVisible(fShow);
}
