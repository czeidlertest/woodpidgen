#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>

#include <profile.h>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(Profile *profile, QWidget *parent = 0);
    ~MainWindow();
    
public slots:
    void accountActionToggled(bool checked);
    void messageActionToggled(bool checked);
private:
    Ui::MainWindow *ui;
    QWidget *fIdentityView;
    QWidget *fMessageView;
    Profile *fProfile;
    QProgressBar *fProgressBar;
};

#endif // MAINWINDOW_H
