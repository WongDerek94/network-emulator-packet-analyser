/*----------------------------------------------------------------------------------------------------------------------------
 * SOURCE FILE:    networkemulator.cpp
 *
 * FUNCTIONS:      void NetworkEmulator::processPendingDatagram()
 *                 void NetworkEmulator::on_startButton_clicked()
 *                 void NetworkEmulator::on_stopButton_clicked()
 *                 bool NetworkEmulator::on_saveButton_clicked()
 *                 void NetworkEmulator::on_resetButton_clicked()
 *                 void NetworkEmulator::onNetworkDelaySliderChange()
 *                 void NetworkEmulator::onBitErrorRateSliderChange()
 *                 QStandardItemModel* NetworkEmulator::convertAbstractModelToStandard(QAbstractItemModel* model)
 *                 void NetworkEmulator::resetFiguresState()
 *                 void NetworkEmulator::init()
 *                 bool NetworkEmulator::dropPkt(int prob)
 *                 void NetworkEmulator::averagePktDelay(int delayInMS)
 *                 void NetworkEmulator::relayPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString)
 *                 void NetworkEmulator::recordPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString)
 *                 void NetworkEmulator::updatePacketTable(struct packet* packet, QHostAddress* sourceIP, quint16 sourcePort, const char* destinationIP, int destinationPort, bool isDropped, QString relTime, QColor rowColor)
 *                 void NetworkEmulator::updateNetworkSummaryTable(QString relTime)
 *                 void NetworkEmulator::updateTimeSequence(struct packet* pkt, QHostAddress* sourceIP, QTime* relTime)
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * NOTES:
 * The file contains functionality of the Network Emulator application
 * ----------------------------------------------------------------------------------------------------------------------------*/

#include "../logger.h"
#include "networkemulator.h"
#include "ui_networkemulator.h"

QT_CHARTS_USE_NAMESPACE

static const int RELATIVE_TIME_INDEX = 0;
static const int WINDOW_SIZE_INDEX = 1;
static const int PACKET_TYPE_INDEX = 2;
static const int RETRANSMIT_INDEX = 3;
static const int SEQUENCE_NUM_INDEX = 4;
static const int ACKNOWLEDGEMENT_NUM_INDEX = 5;
static const int SOURCE_IP_INDEX = 6;
static const int SOURCE_PORT_INDEX = 8;
static const int DESTINATION_IP_INDEX = 7;
static const int DESTINATION_PORT_INDEX = 9;

static const int TRANSMITTER_IP_INDEX = 0;
static const int TRANSMITTER_PORT_INDEX = 1;
static const int RECEIVER_IP_INDEX = 2;
static const int RECEIVER_PORT_INDEX = 3;
static const int NETWORK_EMULATOR_IP_INDEX = 4;
static const int NETWORK_EMULATOR_PORT_INDEX = 5;
static const int PAYLOAD_LEN_INDEX = 6;
static const int MAX_WINDOW_SIZE_INDEX = 7;

static const int totalCaptureTimeIndex = 0;
static const int packetCountIndex = 1;
static const int droppedPacketsIndex = 2;
static const int retransmitIndex = 3;

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::NetworkEmulator
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      NetworkEmulator::NetworkEmulator(QWidget *parent)
 *
 * RETURNS:        an instance of NetworkEmulator
 *
 * NOTES:
 * Constructor of NetworkEmulator class
 * ----------------------------------------------------------------------------------------------------------------------------*/
NetworkEmulator::NetworkEmulator(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::NetworkEmulator)
{

    packetSize = sizeof(struct packet);
    pkt = (struct packet *)malloc(packetSize);

    ui->setupUi(this);
    setWindowTitle("Network Emulator");
    init();
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::~NetworkEmulator
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      NetworkEmulator::~NetworkEmulator()
 *
 * NOTES:
 * Destructor of NetworkEmulator class
 * ----------------------------------------------------------------------------------------------------------------------------*/
NetworkEmulator::~NetworkEmulator()
{
    delete pkt;
    delete ui;
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::processPendingDatagram
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::processPendingDatagram()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Listens for incoming packets on the specified port
 * Applies average delay specified by network delay value
 * Drops a packet with a probability specified by Bit Error Rate (BER)
 * Updates UI
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::processPendingDatagram()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        // read and store datagram
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        pkt = static_cast<struct packet *>((void *)datagram.data());

        // Get relative time since network initialization
        gettimeofday(&end, NULL);
        QTime relTime(0,0);
        relTime = relTime.addMSecs(delay(start, end));
        QString relTimeString = relTime.toString("m:ss:zzz");

        if (pause) continue;
        // Note: Packets are not filtered at this point, they can come from any host
        // Filter only for packets coming from either transmitter or receiver
        if (QString::compare(sender.toString(), TRANSMITTER_IP) == 0 || (QString::compare(sender.toString(), RECEIVER_IP) == 0))
        {
            // Add network delay bi-directionally
            averagePktDelay(networkDelay);

            if (!dropPkt(errorRatePercent))
            {
                relayPacket(&sender, senderPort, &relTime, relTimeString);
            }
            else
            {
                // Update dropped packet on UI but don't forward packet
                recordPacket(&sender, senderPort, &relTime, relTimeString);
            }
            updateNetworkSummaryTable(relTimeString);
        }
    }
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::on_startButton_clicked
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::on_startButton_clicked()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Starts application when the start button is clicked
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::on_startButton_clicked()
{
    pause = false;
    ui->statusLabel->setText(statusLabelTextActive);
    ui->statusLabel->setStyleSheet(statusLabelStyleActive);
    if (start.tv_sec == 0)
    {
        // start timer
        gettimeofday(&start, NULL);
    }
    if (udpSocket == nullptr)
    {
        udpSocket = new QUdpSocket(this);
        udpSocket->bind(QHostAddress(QString(NETWORK_EMULATOR_IP)), NETWORK_EMULATOR_PORT);
        connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));
    }
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::on_stopButton_clicked
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::on_stopButton_clicked()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Stops application when the stop button is clicked
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::on_stopButton_clicked()
{
    pause = true;
    ui->statusLabel->setText(statusLabelTextStopped);
    ui->statusLabel->setStyleSheet(statusLabelStyleStopped);
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::on_saveButton_clicked
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      bool NetworkEmulator::on_saveButton_clicked()
 *
 * RETURNS:        bool
 *
 * NOTES:
 * Saves content of the packet table to a csv file
 * ----------------------------------------------------------------------------------------------------------------------------*/
bool NetworkEmulator::on_saveButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Packets", "packets.csv", "CSV files (.csv)", 0);
    QSaveFile file(filename);
    if (!file.open(QFile::WriteOnly |QFile::Truncate))
    {
        return false;
    }

    QString csvData;
    for (int i = 0; i < ui->packetTable->model()->rowCount(); i++)
    {
        int numColumns = ui->packetTable->model()->columnCount();
        for (int j = 0; j < numColumns; j++)
        {
            QString value = ui->packetTable->model()->data(ui->packetTable->model()->index(i, j)).toString();
            csvData += value;
            if (j != (numColumns -1))
            {
               csvData += ",";
            }
        }
        csvData += "\r\n";
    }

    QByteArray dataArray = csvData.toLocal8Bit();
    char* buffer = dataArray.data();
    if (-1 == file.write(buffer))
    {
        return false;
    }
    return file.commit();
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::on_resetButton_clicked
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::on_resetButton_clicked()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Resets state of the application to the initial
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::on_resetButton_clicked()
{
    pause = true;
    if (QStandardItemModel* packetTableModel = convertAbstractModelToStandard(ui->packetTable->model()))
    {
        packetTableModel->clear();
        delete packetTableModel;
        packetTableModel = nullptr;
    }

    if (QStandardItemModel* networkSummaryTableModel = convertAbstractModelToStandard(ui->networkSummaryTable->model()))
    {
        networkSummaryTableModel->clear();
        delete networkSummaryTableModel;
        networkSummaryTableModel = nullptr;
    }

    resetFiguresState();
    init();
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::onNetworkDelaySliderChange
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::onNetworkDelaySliderChange()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Updates network delay value when the slider is moved
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::onNetworkDelaySliderChange()
{
    networkDelay = ui->packetDelaySlider->value();
    ui->packetDelayLabel->setText("Packet Delay (ms): " + QString::number(networkDelay));
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::onNetworkDelaySliderChange
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::onNetworkDelaySliderChange()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Updates Bit Error Rate (BER) value when the slider is moved
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::onBitErrorRateSliderChange()
{
    errorRatePercent = ui->bitErrorRateSlider->value();
    ui->bitErrorRateLabel->setText("Bit Error Rate: " + QString::number(errorRatePercent) + "%");
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::convertAbstractModelToStandard
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      QStandardItemModel* NetworkEmulator::convertAbstractModelToStandard(QAbstractItemModel* model)
 *
 * RETURNS:        QStandardItemModel*
 *
 * NOTES:
 * Dynamically casts instance QAbstractItemModel to QStandardItemModel
 * ----------------------------------------------------------------------------------------------------------------------------*/
QStandardItemModel* NetworkEmulator::convertAbstractModelToStandard(QAbstractItemModel* model)
{
    QStandardItemModel* standardModel = dynamic_cast<QStandardItemModel*>(model);
    return standardModel;
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::resetFiguresState
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::resetFiguresState()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Resets UI state
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::resetFiguresState()
{
    packetTableRowIndex = 0;
    maxX = INITIAL_MAX_X;
    maxY = INITIAL_MAX_Y;
    start = {0, 0};
    networkDelay = NETWORK_DELAY_MS;
    errorRatePercent = ERROR_RATE_PERCENT;
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::init
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::init()
 *
 * RETURNS:        void
 *
 * NOTES:
 * Initializes UI
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::init()
{
    // Init sliders labels and default values
    ui->packetDelaySlider->setValue(networkDelay);
    ui->packetDelaySlider->setMinimum(MIN_NETWORK_DELAY_MS);
    ui->packetDelaySlider->setMaximum(MAX_NETWORK_DELAY_MS);
    ui->packetDelayLabel->setText("Packet Delay (ms): " + QString::number(networkDelay));
    ui->bitErrorRateSlider->setValue(errorRatePercent);
    ui->bitErrorRateSlider->setMinimum(MIN_ERROR_RATE_PERCENT);
    ui->bitErrorRateSlider->setMaximum(MAX_ERROR_RATE_PERCENT);
    ui->bitErrorRateLabel->setText("Bit Error Rate: " + QString::number(errorRatePercent) + "%");
    connect(ui->packetDelaySlider, SIGNAL(valueChanged(int)), SLOT(onNetworkDelaySliderChange()));
    connect(ui->bitErrorRateSlider, SIGNAL(valueChanged(int)), SLOT(onBitErrorRateSliderChange()));

    // configure status label
    ui->statusLabel->setText(statusLabelTextStopped);
    ui->statusLabel->setStyleSheet(statusLabelStyleStopped);

    // Init and configure time sequence chart
    chart = new QChart();
    series = new QLineSeries();
    chart->legend()->setVisible(false);
    chart->addSeries(series);
    chart->setTitle("Time-Sequence Graph of DATA packets from transmitter to receiver");

    QFont axisFont;
    axisFont.setPixelSize(11);
    axisFont.setBold(true);

    axisX = new QValueAxis;
    axisX->setTitleText("Relative Time (s)");
    axisX->setTitleFont(axisFont);
    axisX->setRange(minX, maxX);
    axisX->setGridLineVisible(true);

    axisY = new QValueAxis;
    axisY->setTitleText("Sequence #");
    axisY->setTitleFont(axisFont);
    axisY->setRange(minY,maxY);
    axisY->setGridLineVisible(true);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // Create chart view
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    ui->timeSequenceChart->setRenderHint(QPainter::Antialiasing);
    ui->timeSequenceChart->setChart(chart);

    // Init Settings table model with loaded configuration values
    settingTableModel = new QStandardItemModel;
    ui->settingsTable->setModel(settingTableModel);
    ui->settingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->settingsTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->settingsTable->horizontalHeader()->setVisible(false);
    ui->settingsTable->verticalHeader()->setVisible(false);

    QString transmitterIP = "Transmitter IP";
    QString transmitterPort = "Transmitter Port";
    QString receiverIP = "Receiver IP";
    QString receiverPort = "Receiver Port";
    QString networkEmulatorIP = "Network Emulator IP";
    QString networkEmulatorPort = "Network Emulator Port";
    QString payloadLen = "Payload Length";
    QString maxWindoSize = "Max Window Size";
    QString transmitterIPValue = TRANSMITTER_IP;
    QString transmitterPortValue = QString::number(TRANSMITTER_PORT);
    QString receiverIPValue = RECEIVER_IP;
    QString receiverPortValue = QString::number(RECEIVER_PORT);
    QString networkEmulatorIPValue = NETWORK_EMULATOR_IP;
    QString networkEmulatorPortValue = QString::number(NETWORK_EMULATOR_PORT);
    QString payloadLenValue = QString::number(PAYLOAD_LEN);
    QString maxWindowSizeValue = QString::number(MAX_WINDOW_SIZE);

    settingTableModel->setItem(TRANSMITTER_IP_INDEX, 0, new QStandardItem(transmitterIP));
    settingTableModel->setItem(TRANSMITTER_PORT_INDEX, 0, new QStandardItem(transmitterPort));
    settingTableModel->setItem(RECEIVER_IP_INDEX, 0, new QStandardItem(receiverIP));
    settingTableModel->setItem(RECEIVER_PORT_INDEX, 0, new QStandardItem(receiverPort));
    settingTableModel->setItem(NETWORK_EMULATOR_IP_INDEX, 0, new QStandardItem(networkEmulatorIP));
    settingTableModel->setItem(NETWORK_EMULATOR_PORT_INDEX, 0, new QStandardItem(networkEmulatorPort));
    settingTableModel->setItem(PAYLOAD_LEN_INDEX, 0, new QStandardItem(payloadLen));
    settingTableModel->setItem(MAX_WINDOW_SIZE_INDEX, 0, new QStandardItem(maxWindoSize));
    settingTableModel->setItem(TRANSMITTER_IP_INDEX, 1, new QStandardItem(transmitterIPValue));
    settingTableModel->setItem(TRANSMITTER_PORT_INDEX, 1, new QStandardItem(transmitterPortValue));
    settingTableModel->setItem(RECEIVER_IP_INDEX, 1, new QStandardItem(receiverIPValue));
    settingTableModel->setItem(RECEIVER_PORT_INDEX, 1, new QStandardItem(receiverPortValue));
    settingTableModel->setItem(NETWORK_EMULATOR_IP_INDEX, 1, new QStandardItem(networkEmulatorIPValue));
    settingTableModel->setItem(NETWORK_EMULATOR_PORT_INDEX, 1, new QStandardItem(networkEmulatorPortValue));
    settingTableModel->setItem(PAYLOAD_LEN_INDEX, 1, new QStandardItem(payloadLenValue));
    settingTableModel->setItem(MAX_WINDOW_SIZE_INDEX, 1, new QStandardItem(maxWindowSizeValue));

    // Init packet table model
    packetTableModel = new QStandardItemModel;
    ui->packetTable->setModel(packetTableModel);
    ui->packetTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QString relativeTimeHeader = "Relative Time";
    QString seqNumHeader = "Seq #";
    QString ackNumHeader = "Ack #";
    QString srcIPHeader = "Source IP";
    QString srcPtHeader = "Source Port";
    QString dstIPHeader = "Destination IP";
    QString destPtHeader = "Destination Port";
    QString pktTypeHeader = "Packet Type";
    QString windowSizeHeader = "Window Size";
    QString retransmitHeader = "Retransmit";
    packetTableModel->setItem(packetTableRowIndex, RELATIVE_TIME_INDEX, new QStandardItem(relativeTimeHeader));
    packetTableModel->setItem(packetTableRowIndex, SEQUENCE_NUM_INDEX, new QStandardItem(seqNumHeader));
    packetTableModel->setItem(packetTableRowIndex, ACKNOWLEDGEMENT_NUM_INDEX, new QStandardItem(ackNumHeader));
    packetTableModel->setItem(packetTableRowIndex, SOURCE_IP_INDEX, new QStandardItem(srcIPHeader));
    packetTableModel->setItem(packetTableRowIndex, DESTINATION_IP_INDEX, new QStandardItem(dstIPHeader));
    packetTableModel->setItem(packetTableRowIndex, SOURCE_PORT_INDEX, new QStandardItem(srcPtHeader));
    packetTableModel->setItem(packetTableRowIndex, DESTINATION_PORT_INDEX, new QStandardItem(destPtHeader));
    packetTableModel->setItem(packetTableRowIndex, PACKET_TYPE_INDEX, new QStandardItem(pktTypeHeader));
    packetTableModel->setItem(packetTableRowIndex, WINDOW_SIZE_INDEX, new QStandardItem(windowSizeHeader));
    packetTableModel->setItem(packetTableRowIndex, RETRANSMIT_INDEX, new QStandardItem(retransmitHeader));
    packetTableRowIndex++;

    // Init network summary statistics model and table
    summaryTableModel = new QStandardItemModel;
    QStringList summaryHeaders = { "Total Capture Time", "Packet Count", "Dropped Packets (DATA, ACK, EOT)", "Retransmits (DATA only)" };
    for(int i = 0; i < summaryHeaders.size(); i++)
    {
       summaryTableModel->setItem(0, i, new QStandardItem(summaryHeaders[i]));
    }

    ui->networkSummaryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->networkSummaryTable->setModel(summaryTableModel);    
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::dropPkt
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      bool NetworkEmulator::dropPkt(int prob)
 *
 * RETURNS:        bool
 *
 * NOTES:
 * Drops packet based on probability
 * ----------------------------------------------------------------------------------------------------------------------------*/
bool NetworkEmulator::dropPkt(int prob)
{
    bool drop = (prob < (rand() % 100) + 1) ? false : true;
    if (drop) ++droppedPackets;
    return drop;
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::averagePktDelay
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::averagePktDelay(int delayInMS)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Applies network delay for each received packet
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::averagePktDelay(int delayInMS)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval future;
    gettimeofday(&future, NULL);

    while(delay(now, future) < delayInMS)
    {
        gettimeofday(&future, NULL);
        continue;
    }
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::relayPacket
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::relayPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Relays a packet to receiver if it came from transmiiter
 * Relays a packet to transmitter if it came from receiver
 * Updates UI
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::relayPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString)
{
    QColor rowColor;

    if (QString::compare(sender->toString(), TRANSMITTER_IP) == 0 && senderPort == TRANSMITTER_PORT)
    {
        if (pkt->retransmit == true) ++retransmits;
        // Send to Receiver
        if (pkt->packetType == EOT)
        {
            rowColor = QColor(148, 134, 131, 75);
        }
        else
        {
            rowColor = QColor(241, 124, 14, 75);
        }
        updateTimeSequence(pkt, sender, relTime);
        updatePacketTable(pkt, sender, senderPort, RECEIVER_IP, RECEIVER_PORT, false, relTimeString, rowColor);
        if(udpSocket->writeDatagram((const char*)pkt, packetSize, QHostAddress(RECEIVER_IP), RECEIVER_PORT) != packetSize)
        {
            logToFile(static_cast<LogType>(ERROR), NULL, "sendto error");
            exit(1);
        }
        if (pkt->seqNum != INVALID_SEQ_NUM)
        {
            logToFile(static_cast<LogType>(INFO), pkt, "transmitter->receiver (seqNum: %d)", pkt->seqNum);
        }
        else
        {
            logToFile(static_cast<LogType>(INFO), pkt, "transmitter->receiver (EOT)");
        }
    }
    else if (QString::compare(sender->toString(), RECEIVER_IP) == 0 && senderPort == RECEIVER_PORT)
    {
        // Send to Transmitter
        rowColor = QColor(0, 60, 121, 75);
        updatePacketTable(pkt, sender, senderPort, TRANSMITTER_IP, TRANSMITTER_PORT, false, relTimeString, rowColor);
        if (udpSocket->writeDatagram((const char*)pkt, packetSize, QHostAddress(TRANSMITTER_IP), TRANSMITTER_PORT) != packetSize)
        {
            logToFile(static_cast<LogType>(ERROR), NULL, "sendto error");
            exit(1);
        }
        logToFile(static_cast<LogType>(INFO), pkt, "receiver->transmitter (ackNum: %d)", pkt->ackNum);
    }
    else
    {
        logToFile(static_cast<LogType>(ERROR), NULL, "unknown client, skipping packet");
    }
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::recordPacket
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::recordPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Updates UI with a new packet data
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::recordPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString)
{
    QColor rowColor;

    if (QString::compare(sender->toString(), TRANSMITTER_IP) == 0 && senderPort == TRANSMITTER_PORT)
    {
        // Send to Receiver
        if (pkt->packetType == EOT)
        {
            rowColor = QColor(148, 134, 131, 75);
        }
        else
        {
            rowColor = QColor(241, 124, 14, 75);
        }
        updateTimeSequence(pkt, sender, relTime);
        updatePacketTable(pkt, sender, senderPort, RECEIVER_IP, RECEIVER_PORT, true, relTimeString, rowColor);
        if (pkt->seqNum != INVALID_SEQ_NUM)
        {
            logToFile(static_cast<LogType>(INFO), pkt, "DROPPED: transmitter->receiver (seqNum: %d)", pkt->seqNum);
        }
        else
        {
            logToFile(static_cast<LogType>(INFO), pkt, "DROPPED: transmitter->receiver (EOT)");
        }
    }
    else if (QString::compare(sender->toString(), RECEIVER_IP) == 0 && senderPort == RECEIVER_PORT)
    {
        // Send to Transmitter
        rowColor = QColor(0, 60, 121, 75);
        updatePacketTable(pkt, sender, senderPort, TRANSMITTER_IP, TRANSMITTER_PORT, true, relTimeString, rowColor);
        logToFile(static_cast<LogType>(INFO), pkt, "DROPPED: receiver->transmitter (ackNum: %d)", pkt->ackNum);
    }
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::updatePacketTable
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::updatePacketTable(struct packet* packet, QHostAddress* sourceIP,
 *                     quint16 sourcePort, const char* destinationIP, int destinationPort, bool isDropped, QString relTime, QColor rowColor
 *                 )
 *
 * RETURNS:        void
 *
 * NOTES:
 * Updates packet table with a new packet data
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::updatePacketTable(struct packet* packet, QHostAddress* sourceIP, quint16 sourcePort, const char* destinationIP, int destinationPort, bool isDropped, QString relTime, QColor rowColor)
{
    QString ackNum = QString::number(packet->ackNum);
    QString seqNum = QString::number(packet->seqNum);
    QString srcIP = sourceIP->toString();
    QString srcPt = QString::number(sourcePort);
    QString dstIP = destinationIP;
    QString destPt = QString::number(destinationPort);
    QString pktType = packetTypeToString(packet->packetType, isDropped);
    QString windowSize = QString::number(packet->windowSize);
    QString retransmit = (packet->retransmit == true) ? "Yes" : "No";

    // Set Packet Table UI modifications
    packetTableModel->setItem(packetTableRowIndex, RELATIVE_TIME_INDEX, new QStandardItem(relTime));
    packetTableModel->setItem(packetTableRowIndex, SEQUENCE_NUM_INDEX, new QStandardItem(seqNum));
    packetTableModel->setItem(packetTableRowIndex, ACKNOWLEDGEMENT_NUM_INDEX, new QStandardItem(ackNum));
    packetTableModel->setItem(packetTableRowIndex, SOURCE_IP_INDEX, new QStandardItem(srcIP));
    packetTableModel->setItem(packetTableRowIndex, DESTINATION_IP_INDEX, new QStandardItem(dstIP));
    packetTableModel->setItem(packetTableRowIndex, SOURCE_PORT_INDEX, new QStandardItem(srcPt));
    packetTableModel->setItem(packetTableRowIndex, DESTINATION_PORT_INDEX, new QStandardItem(destPt));
    packetTableModel->setItem(packetTableRowIndex, PACKET_TYPE_INDEX, new QStandardItem(pktType));
    packetTableModel->setItem(packetTableRowIndex, WINDOW_SIZE_INDEX, new QStandardItem(windowSize));
    packetTableModel->setItem(packetTableRowIndex, RETRANSMIT_INDEX, new QStandardItem(retransmit));

    // set Packet Table row color
    for (int i = 0; i <= DESTINATION_PORT_INDEX; i++)
    {
        QModelIndex index = packetTableModel->index(NetworkEmulator::packetTableRowIndex, i);
        packetTableModel->setData(index, rowColor, Qt::BackgroundRole);
    }

    NetworkEmulator::packetTableRowIndex++;
    ui->packetTable->scrollToBottom();
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::updateNetworkSummaryTable
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::updateNetworkSummaryTable(QString relTime)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Updates summary table
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::updateNetworkSummaryTable(QString relTime)
{
    summaryTableModel->setItem(1, totalCaptureTimeIndex, new QStandardItem(relTime));
    summaryTableModel->setItem(1, packetCountIndex, new QStandardItem(QString::number(packetTableRowIndex-1)));
    summaryTableModel->setItem(1, droppedPacketsIndex, new QStandardItem(QString::number(droppedPackets)));
    summaryTableModel->setItem(1, retransmitIndex, new QStandardItem(QString::number(retransmits)));
}

/*----------------------------------------------------------------------------------------------------------------------------
 * FUNCTION:       NetworkEmulator::updateTimeSequence
 *
 * DATE:           December 3rd, 2020
 *
 * REVISIONS:      N/A
 *
 * DESIGNER:       Derek Wong, Maksym Chumak
 *
 * PROGRAMMER:     Derek Wong, Maksym Chumak
 *
 * INTERFACE:      void NetworkEmulator::updateTimeSequence(struct packet* pkt, QHostAddress* sourceIP, QTime* relTime)
 *
 * RETURNS:        void
 *
 * NOTES:
 * Updates Time Sequence graph
 * ----------------------------------------------------------------------------------------------------------------------------*/
void NetworkEmulator::updateTimeSequence(struct packet* pkt, QHostAddress* sourceIP, QTime* relTime)
{
    // Only add data from transmitter to receiver to time sequence chart
    if (QString::compare(sourceIP->toString(), TRANSMITTER_IP) == 0)
    {
        double totalSeconds = 0;
        double minutes = relTime->minute();
        double seconds = relTime->second();
        totalSeconds = minutes * 60 + seconds;

        int seqNum = pkt->seqNum;
        if(pkt->packetType == DATA)
        {
            series->append(totalSeconds, seqNum);
            if (maxX < totalSeconds)
            {
                maxX = totalSeconds;
                axisX->setMax(maxX+1);
            }
            else if(maxY < seqNum)
            {
                maxY = seqNum;
                axisY->setMax(maxY+1);
            }
        }
    }
}
