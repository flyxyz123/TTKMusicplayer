#include "musicconnecttransferwidget.h"
#include "ui_musicconnecttransferwidget.h"
#include "musicbackgroundmanager.h"
#include "musicuiobject.h"
#include "musicconnectionpool.h"
#include "musicsettingmanager.h"
#include "musicmessagebox.h"
#include "musicprogresswidget.h"

#include <QFile>
#include <QPushButton>
#include <QButtonGroup>
#ifdef Q_OS_WIN
#   include <windows.h>
#endif

MusicConnectTransferWidget::MusicConnectTransferWidget(QWidget *parent)
    : MusicAbstractMoveDialog(parent),
      ui(new Ui::MusicConnectTransferWidget)
{
    ui->setupUi(this);

    m_currentIndex = -1;

    ui->topTitleCloseButton->setIcon(QIcon(":/share/searchclosed"));
    ui->topTitleCloseButton->setStyleSheet(MusicUIObject::MToolButtonStyle03);
    ui->topTitleCloseButton->setCursor(QCursor(Qt::PointingHandCursor));
    ui->topTitleCloseButton->setToolTip(tr("Close"));
    connect(ui->topTitleCloseButton, SIGNAL(clicked()), SLOT(close()));

    ui->allSelectedcheckBox->setStyleSheet(MusicUIObject::MCheckBoxStyle02);
    ui->allSelectedcheckBox->setCursor(QCursor(Qt::PointingHandCursor));
    ui->allSelectedcheckBox->setText(tr("allselected"));
    connect(ui->allSelectedcheckBox, SIGNAL(clicked(bool)), SLOT(selectedAllItems(bool)));

    ui->reflashButton->setStyleSheet(MusicUIObject::MPushButtonStyle08);
    ui->reflashButton->setCursor(QCursor(Qt::PointingHandCursor));
    connect(ui->reflashButton, SIGNAL(clicked()), SLOT(reflashRemovableDir()));

    ui->transferButton->setStyleSheet(MusicUIObject::MPushButtonStyle08);
    ui->transferButton->setCursor(QCursor(Qt::PointingHandCursor));
    connect(ui->transferButton, SIGNAL(clicked()), SLOT(startToTransferFiles()));

    connect(ui->searchLineEdit, SIGNAL(cursorPositionChanged(int,int)), SLOT(musicSearchIndexChanged(int,int)));
    M_CONNECTION_PTR->setValue("MusicConnectTransferWidget", this);
    M_CONNECTION_PTR->poolConnect("MusicConnectTransferWidget", "MusicSongsSummarizied");
}

MusicConnectTransferWidget::~MusicConnectTransferWidget()
{
    M_CONNECTION_PTR->poolDisConnect("MusicConnectTransferWidget");
    delete ui;
}

void MusicConnectTransferWidget::initColumns()
{
    MusicSongsList songs;
    QStringList names;
    emit getMusicLists(songs, names);

    QButtonGroup *group = new QButtonGroup(this);
    connect(group, SIGNAL(buttonClicked(int)), SLOT(currentPlayListSelected(int)));
    for(int i=0; i<songs.count(); ++i)
    {
        QPushButton *button = new QPushButton(QString("%1(%2)")
                                              .arg(names[i]).arg(songs[i].count()), this);
        button->setStyleSheet(MusicUIObject::MPushButtonStyle08);
        button->setCursor(QCursor(Qt::PointingHandCursor));
        button->setGeometry(32, 100 + 50*i, 90, 25);
        group->addButton(button, i);
    }

    reflashRemovableDir();
}

void MusicConnectTransferWidget::createAllItems(const MusicSongs &songs)
{
    ui->playListTableWidget->clear();
    if(ui->allSelectedcheckBox->isChecked())
    {
        ui->allSelectedcheckBox->click();
    }
    ui->playListTableWidget->setRowCount(songs.count());
    for(int i=0; i<songs.count(); ++i)
    {
        MusicSong song = songs[i];
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setData(MUSIC_CHECK_ROLE, false);
        ui->playListTableWidget->setItem(i, 0, item);

                          item = new QTableWidgetItem;
        item->setText(MusicUtils::elidedText(font(), song.getMusicName(), Qt::ElideRight, 280));
        item->setToolTip(song.getMusicPath());
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->playListTableWidget->setItem(i, 1, item);

                item = new QTableWidgetItem;
        item->setText(MusicUtils::elidedText(font(), song.getMusicTime(), Qt::ElideRight, 40));
        item->setToolTip(song.getMusicTime());
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->playListTableWidget->setItem(i, 2, item);
    }
}

QString MusicConnectTransferWidget::getRemovableDrive()
{
#ifdef Q_OS_WIN
    QFileInfoList drives = QDir::drives();
    foreach(QFileInfo driver, drives)
    {
        QString path = driver.absoluteDir().absolutePath();
        if(GetDriveTypeA(path.toStdString().c_str()) == DRIVE_REMOVABLE)
        {
            return path;
        }
    }
#endif
    return QString();
}

void MusicConnectTransferWidget::currentPlayListSelected(int index)
{
    MusicSongsList songs;
    QStringList names;
    emit getMusicLists(songs, names);
    if(index >= songs.count())
    {
        return;
    }

    m_searchfileListCache.clear();
    ui->searchLineEdit->clear();
    m_currentSongs = songs[m_currentIndex = index];
    createAllItems(m_currentSongs);
}

void MusicConnectTransferWidget::selectedAllItems(bool check)
{
    for(int i=0; i<ui->playListTableWidget->rowCount(); ++i)
    {
        ui->playListTableWidget->item(i, 0)->setData(MUSIC_CHECK_ROLE, check);
    }

    if(!check)
    {
        ui->allSelectedcheckBox->setText(tr("allselected"));
        ui->playListTableWidget->clearSelection();
        ui->playListTableWidget->setCurrentIndex(QModelIndex());
    }
    else
    {
        ui->allSelectedcheckBox->setText(tr("allcanceled"));
        ui->playListTableWidget->selectAll();
    }
}

void MusicConnectTransferWidget::startToTransferFiles()
{
    MusicObject::MIntList list(ui->playListTableWidget->getSelectedItems());
    if(list.isEmpty())
    {
        MusicMessageBox message;
        message.setText(tr("please select one item"));
        message.exec();
        return;
    }

    if(m_currentIndex == -1 || m_currentIndex > m_currentSongs.count())
    {
        return;
    }

    QStringList names;
    foreach(int index, list)
    {
        if(!m_searchfileListCache.value(0).isEmpty())
        {
            int count = ui->searchLineEdit->text().trimmed().count();
            index = m_searchfileListCache.value(count)[index];
        }
        names << m_currentSongs[index].getMusicPath();
    }

    QString path = M_SETTING_PTR->value(MusicSettingManager::MobileDevicePathChoiced).toString();
    if(path.isEmpty())
    {
        path = getRemovableDrive();
    }
    MusicProgressWidget progress;
    progress.show();
    progress.setTitle(tr("Copy File Mode"));
    progress.setRange(0, names.count());
    for(int i=0; i<names.count(); ++i)
    {
        QFile::copy(names[i], QString("%1/%2").arg(path).arg(names[i].split("/").last()));
        progress.setValue(i);
    }

    ui->allSelectedcheckBox->setChecked(false);
    ui->playListTableWidget->clearSelection();
}

void MusicConnectTransferWidget::reflashRemovableDir()
{
    QString path = M_SETTING_PTR->value(MusicSettingManager::MobileDevicePathChoiced).toString();
    if(path.isEmpty())
    {
        path = getRemovableDrive();
    }
    ui->textLabel->setText(QString("( %1 )").arg(path));
    ui->transferButton->setEnabled( !path.isEmpty() );
}

void MusicConnectTransferWidget::musicSearchIndexChanged(int, int index)
{
    MusicObject::MIntList searchResult;
    for(int j=0; j<m_currentSongs.count(); ++j)
    {
        if(m_currentSongs[j].getMusicName().contains(ui->searchLineEdit->text().trimmed(), Qt::CaseInsensitive))
        {
            searchResult << j;
        }
    }
    m_searchfileListCache.insert(index, searchResult);

    MusicSongs songs;
    foreach(int index, searchResult)
    {
        songs.append(m_currentSongs[index]);
    }
    createAllItems(songs);
}

int MusicConnectTransferWidget::exec()
{
    initColumns();
    QPixmap pix(M_BACKGROUND_PTR->getMBackground());
    ui->background->setPixmap(pix.scaled( size() ));
    return MusicAbstractMoveDialog::exec();
}
