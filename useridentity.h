#ifndef USERIDENTITY_H
#define USERIDENTITY_H


#include <QString>


class Contact {
public:
    virtual QString         getSurename();
    virtual QString         getFamilyname();
    //virtual QBitmap         getIcon();

    virtual int             setSurename(const QString& name);
    virtual int             setFamilyname(const QString& name);
    //virtual int             setIcon(const QBitmap& bitmap);

};

class UserIdentity //: public Contact
{
public:
                    UserIdentity();
    

    // keys

    // contacts


    // channels

};

#endif // USERIDENTITY_H
