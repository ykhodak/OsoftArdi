#include "ard-iphone.h"
#import <Foundation/Foundation.h>
#import <AddressBook/AddressBook.h>
#import <UIKit/UIKit.h>
#include "test-view.h"
//#import <UIKit>
//#import <ContactsUI/ContactsUI.h>
//#import <Contacts/Contacts.h>

QString qt_mac_NSStringToQString(const NSString *nsstr)
{
    NSRange range;
    range.location = 0;
    range.length = [nsstr length];
   // QString result(range.length, QChar(0));
 
    unichar *chars = new unichar[range.length];
    [nsstr getCharacters:chars range:range];
    QString r2 = QString::fromUtf16(chars, range.length);
    delete[] chars;
    return r2;
}

int mac_testInt()
{
	return 123;
};

QString mac_testString()
{
	NSString* s;
	s = @"NS-test-string";
	QString rv = qt_mac_NSStringToQString(s);
	return rv;
}

void mac_showSettings()
{
	[[UIApplication sharedApplication] openURL:[NSURL URLWithString:UIApplicationOpenSettingsURLString]];
}
