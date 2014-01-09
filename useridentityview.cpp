#include "useridentityview.h"

#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "profile.h"
#include "useridentity.h"

class UserIdentityDetailsView : public QWidget
{
public:
    explicit UserIdentityDetailsView(QWidget *parent = 0);

    void setTo(UserIdentity *identity);
private:
    UserIdentity *fIdentity;
    QLineEdit *fServerUserName;
};

UserIdentityDetailsView::UserIdentityDetailsView(QWidget *parent) :
    QWidget(parent),
    fIdentity(NULL)
{
    fServerUserName = new QLineEdit(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(fServerUserName);
}

void UserIdentityDetailsView::setTo(UserIdentity *identity)
{
    fIdentity = identity;
    fServerUserName->setText(fIdentity->getMyself()->getServerUser());
}

UserIdentityView::UserIdentityView(IdentityListModel *listModel, QWidget *parent) :
    QSplitter(Qt::Horizontal, parent),
    fIdentityListModel(listModel)
{
    // left identity list
    QWidget *identityListWidget = new QWidget(this);
    QVBoxLayout *identityListLayout = new QVBoxLayout();
    identityListWidget->setLayout(identityListLayout);

    fIdentityList = new QListView(identityListWidget);
    fIdentityList->setViewMode(QListView::ListMode);
    fIdentityList->setModel(fIdentityListModel);
    connect(fIdentityList, SIGNAL(clicked(QModelIndex)), this, SLOT(onIdentitySelected(QModelIndex)));

    // add remove buttons
    QHBoxLayout* manageIdentitiesLayout = new QHBoxLayout();
    fAddIdentity = new QPushButton("Create", identityListWidget);
    fRemoveIdentity = new QPushButton("Remove", identityListWidget);
    manageIdentitiesLayout->addStretch();
    manageIdentitiesLayout->addWidget(fAddIdentity);
    manageIdentitiesLayout->addWidget(fRemoveIdentity);

    identityListLayout->addWidget(fIdentityList);
    identityListLayout->addItem(manageIdentitiesLayout);

    // right side
    fDisplayStack = new QStackedWidget(this);
    fDetailView = new UserIdentityDetailsView(fDisplayStack);
    fDisplayStack->addWidget(fDetailView);

    addWidget(identityListWidget);
    addWidget(fDisplayStack);
}

void UserIdentityView::onIdentitySelected(QModelIndex index)
{
    UserIdentity *identity = fIdentityListModel->identityAt(index.row());
    fDetailView->setTo(identity);
}

