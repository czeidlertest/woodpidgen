#ifndef USERIDENTITYVIEW_H
#define USERIDENTITYVIEW_H

#include <QSplitter>

class QListView;
class QPushButton;
class QStackedWidget;

class UserIdentityDetailsView;
class IdentityListModel;

class UserIdentityView : public QSplitter
{
    Q_OBJECT
public:
    explicit UserIdentityView(IdentityListModel *listModel, QWidget *parent = 0);
    
signals:
    
public slots:
    
private:
    QListView* fIdentityList;
    QPushButton *fAddIdentity;
    QPushButton *fRemoveIdentity;
    IdentityListModel *fIdentityListModel;

    QStackedWidget *fDisplayStack;
    UserIdentityDetailsView *fDetailView;
};

#endif // USERIDENTITYVIEW_H
