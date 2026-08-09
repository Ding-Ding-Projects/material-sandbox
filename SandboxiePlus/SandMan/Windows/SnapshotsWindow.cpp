#include "stdafx.h"
#include "SnapshotsWindow.h"
#include "M3DialogHost.h"
#include "SandMan.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../MiscHelpers/Common/TreeItemModel.h"
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>


CSnapshotsWindow::CSnapshotsWindow(const CSandBoxPtr& pBox, QWidget *parent)
	: QDialog(parent)
{
	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::CustomizeWindowHint;
	//flags &= ~Qt::WindowContextHelpButtonHint;
	//flags &= ~Qt::WindowSystemMenuHint;
	//flags &= ~Qt::WindowMinMaxButtonsHint;
	flags |= Qt::WindowMinimizeButtonHint;
	//flags &= ~Qt::WindowCloseButtonHint;
	setWindowFlags(flags);

	this->setWindowFlag(Qt::WindowStaysOnTopHint, theGUI->IsAlwaysOnTop());

	setWindowTitle(tr("%1 - Snapshots").arg(pBox->GetName()));
	ui.treeSnapshots = new QTreeView(this);
	ui.groupBox = new QGroupBox(tr("Selected Snapshot Details"), this);
	ui.txtName = new QLineEdit(ui.groupBox);
	ui.chkDefault = new QCheckBox(tr("Default snapshot"), ui.groupBox);
	ui.chkDefault->setToolTip(tr("When deleting a snapshot content, it will be returned to this snapshot instead of none."));
	ui.txtInfo = new QPlainTextEdit(ui.groupBox);
	ui.btnTake = new QPushButton(tr("Take Snapshot"), this);
	ui.btnSelect = new QToolButton(this);
	ui.btnSelect->setText(tr("Go to Snapshot"));
	ui.btnRemove = new QPushButton(tr("Remove Snapshot"), this);
	QGridLayout* details = new QGridLayout(ui.groupBox);
	details->addWidget(new QLabel(tr("Name:"), ui.groupBox), 0, 0);
	details->addWidget(ui.txtName, 0, 1);
	details->addWidget(ui.chkDefault, 0, 2);
	details->addWidget(new QLabel(tr("Description:"), ui.groupBox), 1, 0, 2, 1, Qt::AlignTop);
	details->addWidget(ui.txtInfo, 1, 1, 2, 2);
	QGroupBox* actions = new QGroupBox(tr("Snapshot Actions"), this);
	QVBoxLayout* actionLayout = new QVBoxLayout(actions);
	actionLayout->addWidget(ui.btnTake);
	actionLayout->addWidget(ui.btnSelect);
	actionLayout->addWidget(ui.btnRemove);
	actionLayout->addStretch();
	QGridLayout* rootGrid = new QGridLayout();
	rootGrid->addWidget(ui.treeSnapshots, 0, 0, 1, 2);
	rootGrid->addWidget(ui.groupBox, 1, 0);
	rootGrid->addWidget(actions, 1, 1);
	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(16, 16, 16, 16);
	root->addLayout(rootGrid);
	M3DialogHost::Install(this);

	ui.treeSnapshots->setAlternatingRowColors(theConf->GetBool("Options/AltRowColors", false));


	m_pBox = pBox;
	m_SaveInfoPending = 0;

	ui.treeSnapshots->setItemDelegate(new CTreeItemDelegate());
	ui.treeSnapshots->setExpandsOnDoubleClick(false);

	m_pSnapshotModel = new CSimpleTreeModel(this);
	m_pSnapshotModel->AddColumn(tr("Snapshot"), "Name");
	m_pSnapshotModel->AddColumn(tr("Creation Time"), "DateFormatted");

	/*m_pSortProxy = new CSortFilterProxyModel(this);
	m_pSortProxy->setSortRole(Qt::EditRole);
	m_pSortProxy->setSourceModel(m_pSnapshotModel);
	m_pSortProxy->setDynamicSortFilter(true);*/

	//ui.treeSnapshots->setItemDelegate(theGUI->GetItemDelegate());

	//ui.treeSnapshots->setModel(m_pSortProxy);
	ui.treeSnapshots->setModel(m_pSnapshotModel);

	connect(ui.treeSnapshots, SIGNAL(clicked(const QModelIndex&)), this, SLOT(UpdateSnapshot(const QModelIndex&)));
	connect(ui.treeSnapshots->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(UpdateSnapshot(const QModelIndex&)));
	connect(ui.treeSnapshots, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnSelectSnapshot()));

	
	QMenu* pSelMenu = new QMenu(ui.btnSelect);
	pSelMenu->addAction(tr("Revert to empty box"), this, SLOT(OnSelectEmpty()));
	ui.btnSelect->setPopupMode(QToolButton::MenuButtonPopup);
	ui.btnSelect->setMenu(pSelMenu);

	connect(ui.btnTake, SIGNAL(clicked(bool)), this, SLOT(OnTakeSnapshot()));
	connect(ui.btnSelect, SIGNAL(clicked(bool)), this, SLOT(OnSelectSnapshot()));
	connect(ui.btnRemove, SIGNAL(clicked(bool)), this, SLOT(OnRemoveSnapshot()));
	
	connect(ui.txtName, SIGNAL(textEdited(const QString&)), this, SLOT(SaveInfo()));
	connect(ui.chkDefault, SIGNAL(clicked(bool)), this, SLOT(OnChangeDefault()));
	connect(ui.txtInfo, SIGNAL(textChanged()), this, SLOT(SaveInfo()));

	ui.groupBox->setEnabled(false);
	ui.btnSelect->setEnabled(false);
	ui.btnRemove->setEnabled(false);

	//statusBar();

	restoreGeometry(theConf->GetBlob("SnapshotsWindow/Window_Geometry"));

	for (int i = 0; i < m_pSnapshotModel->columnCount(); i++)
		m_pSnapshotModel->SetColumnEnabled(i, true);

	UpdateSnapshots(true);
}

CSnapshotsWindow::~CSnapshotsWindow()
{
	theConf->SetBlob("SnapshotsWindow/Window_Geometry",saveGeometry());
}

void CSnapshotsWindow::closeEvent(QCloseEvent *e)
{
	emit Closed();
	this->deleteLater();
}

void CSnapshotsWindow::UpdateSnapshots(bool AndSelect)
{
	m_SnapshotMap.clear();
	QMap<QString, SBoxSnapshot> Snapshots = m_pBox->GetSnapshots(&m_CurSnapshot, &m_DefaultSnapshot);
	foreach(const SBoxSnapshot& Snapshot, Snapshots)
	{
		QVariantMap BoxSnapshot;
		BoxSnapshot["ID"] = Snapshot.ID;
		BoxSnapshot["ParentID"] = Snapshot.Parent;
		if(m_DefaultSnapshot == Snapshot.ID)
			BoxSnapshot["Name"] = Snapshot.NameStr + tr(" (default)");
		else
			BoxSnapshot["Name"] = Snapshot.NameStr;
		BoxSnapshot["Info"] = Snapshot.InfoStr;
		BoxSnapshot["Date"] = Snapshot.SnapDate;
		BoxSnapshot["DateFormatted"] = Snapshot.SnapDate.toString("yyyy-MM-dd hh:mm");
		if(m_CurSnapshot == Snapshot.ID)
			BoxSnapshot["IsBold"] = true;
		m_SnapshotMap.insert(Snapshot.ID, BoxSnapshot);
	}
	m_pSnapshotModel->Sync(m_SnapshotMap);
	ui.treeSnapshots->expandAll();

	if (ui.treeSnapshots->header()) {
		QTimer::singleShot(0, this, [this]() {
			int totalWidth = ui.treeSnapshots->width();
			ui.treeSnapshots->header()->resizeSection(0, totalWidth * 0.7);
			ui.treeSnapshots->header()->resizeSection(1, totalWidth * 0.3);
		});
		ui.treeSnapshots->header()->setSectionResizeMode(0, QHeaderView::Interactive);
		ui.treeSnapshots->header()->setSectionResizeMode(1, QHeaderView::Interactive);
		ui.treeSnapshots->header()->setMinimumSectionSize(100);
		ui.treeSnapshots->header()->setSortIndicatorShown(false);
	}

	if (AndSelect)
	{
		QModelIndex CurIndex = m_pSnapshotModel->FindIndex(m_CurSnapshot);
		if (CurIndex.isValid()) {
			ui.treeSnapshots->selectionModel()->select(CurIndex, QItemSelectionModel::ClearAndSelect);
			UpdateSnapshot(CurIndex);
		}
	}
}

void CSnapshotsWindow::UpdateSnapshot(const QModelIndex& Index)
{
	if (Index.isValid())
	{
		ui.groupBox->setEnabled(true);
		ui.btnSelect->setEnabled(true);
		ui.btnRemove->setEnabled(true);
	}

	//QModelIndex Index = ui.treeSnapshots->currentIndex();
	//QModelIndex ModelIndex = m_pSortProxy->mapToSource(Index);
	//QVariant ID = m_pSnapshotModel->GetItemID(ModelIndex);
	QVariant ID = m_pSnapshotModel->GetItemID(Index);

	OnSaveInfo();
	m_SelectedID = ID;

	QVariantMap BoxSnapshot = m_SnapshotMap[ID];

	m_SaveInfoPending = -1;
	ui.txtName->setText(BoxSnapshot["Name"].toString());
	ui.chkDefault->setChecked(ID == m_DefaultSnapshot);
	ui.txtInfo->setPlainText(BoxSnapshot["Info"].toString());
	m_SaveInfoPending = 0;

	//statusBar()->showMessage(tr("Snapshot: %1 taken: %2").arg(BoxSnapshot["Name"].toString()).arg(BoxSnapshot["Date"].toDateTime().toString()));
}

void CSnapshotsWindow::SaveInfo()
{
	if (m_SaveInfoPending)
		return;
	m_SaveInfoPending = 1;
	QTimer::singleShot(500, this, SLOT(OnSaveInfo()));
}

void CSnapshotsWindow::OnSaveInfo()
{
	if (m_SaveInfoPending != 1)
		return;
	m_SaveInfoPending = 0;

	m_pBox->SetSnapshotInfo(m_SelectedID.toString(), ui.txtName->text(), ui.txtInfo->toPlainText());
	UpdateSnapshots();
}

void CSnapshotsWindow::OnTakeSnapshot()
{
	QString Value = QInputDialog::getText(this, "Sandboxie-Plus", tr("Please enter a name for the new Snapshot."), QLineEdit::Normal, tr("New Snapshot"));
	if (Value.isEmpty())
		return;

	HandleResult(m_pBox->TakeSnapshot(Value));

	UpdateSnapshots(true);
}

void CSnapshotsWindow::OnSelectSnapshot()
{
	QVariant ID = GetCurrentItem();

	SelectSnapshot(ID.toString());
}

void CSnapshotsWindow::OnSelectEmpty()
{
	SelectSnapshot(QString());
}

void CSnapshotsWindow::SelectSnapshot(const QString& ID)
{
	if (QMessageBox("Sandboxie-Plus", tr("Do you really want to switch the active snapshot? Doing so will delete the current state!"), QMessageBox::Question, QMessageBox::Yes, QMessageBox::No | QMessageBox::Default | QMessageBox::Escape, QMessageBox::NoButton, this).exec() != QMessageBox::Yes)
		return;

	HandleResult(m_pBox->SelectSnapshot(ID));
}

void CSnapshotsWindow::OnChangeDefault()
{
	QVariant ID = GetCurrentItem();

	if (ui.chkDefault->isChecked())
		m_DefaultSnapshot = ID.toString();
	else
		m_DefaultSnapshot.clear();

	m_pBox->SetDefaultSnapshot(m_DefaultSnapshot);

	UpdateSnapshots();
}

QVariant CSnapshotsWindow::GetCurrentItem()
{
	QModelIndex Index = ui.treeSnapshots->currentIndex();
	if (!Index.isValid() && !ui.treeSnapshots->selectionModel()->selectedIndexes().isEmpty())
		Index = ui.treeSnapshots->selectionModel()->selectedIndexes().first();
	//QModelIndex ModelIndex = m_pSortProxy->mapToSource(Index);
	//QVariant ID = m_pSnapshotModel->GetItemID(ModelIndex);
	return m_pSnapshotModel->GetItemID(Index);
}

void CSnapshotsWindow::OnRemoveSnapshot()
{
	QVariant ID = GetCurrentItem();

	if (QMessageBox("Sandboxie-Plus", tr("Do you really want to delete the selected snapshot?"), QMessageBox::Question, QMessageBox::Yes, QMessageBox::No | QMessageBox::Default | QMessageBox::Escape, QMessageBox::NoButton, this).exec() != QMessageBox::Yes)
		return;

	ui.groupBox->setEnabled(false);
	ui.btnSelect->setEnabled(false);
	ui.btnRemove->setEnabled(false);

	HandleResult(m_pBox->RemoveSnapshot(ID.toString()));
}

void CSnapshotsWindow::HandleResult(SB_PROGRESS Status)
{
	if (Status.GetStatus() == OP_ASYNC)
	{
		connect(Status.GetValue().data(), SIGNAL(Finished()), this, SLOT(UpdateSnapshots()));
		theGUI->AddAsyncOp(Status.GetValue(), false, tr("Performing Snapshot operation..."), this);
	}
	else if (Status.IsError())
		theGUI->CheckResults(QList<SB_STATUS>() << Status, this);
	UpdateSnapshots();
}
