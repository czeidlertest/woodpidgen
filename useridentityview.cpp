#include "useridentityview.h"

#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <profile.h>


class UserIdentityDetailsView : public QWidget
{
public:
    explicit UserIdentityDetailsView(QWidget *parent = 0);

    void setTo(UserIdentity *identity);
private:
    UserIdentity *fIdentity;
    QLineEdit *fIdentityName;
};

UserIdentityDetailsView::UserIdentityDetailsView(QWidget *parent) :
    QWidget(parent),
    fIdentity(NULL)
{
    fIdentityName = new QLineEdit(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(fIdentityName);
}

void UserIdentityDetailsView::setTo(UserIdentity *identity)
{
    fIdentity = identity;
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

