#include <QFileDialog>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

#include <QApplication>
#include <QGestureEvent> 

#include "utils.h"
#include "utils-img.h"
#include "anfolder.h"
#include "MainWindow.h"
#include "OutlineMain.h"
#include "ardmodel.h"

#define INVALID_IMG QImage()

extern void clear_limbo_file_set();
extern void setSelectLastImageRequest();

QImage load_media(QString image_source_path, int quality)
{
    if (!QFile::exists(image_source_path)) {
        qWarning() << "load_media failed" << image_source_path;
        return INVALID_IMG;
    }

    QImageReader r;
    r.setFileName(image_source_path);
    r.setQuality(quality);
    if (!r.canRead()) {
        ASSERT(0, "load_media - can't read") << image_source_path;
        return INVALID_IMG;
    }

    QImage newImg = r.read();
    return newImg;
}

QImage load_scaled_media(QString image_source_path, int scale2_w, int scale2_h)
{
    if(!QFile::exists(image_source_path)){
        qWarning() << "load_scaled_media failed" << image_source_path;
        return INVALID_IMG;
    }

    QImageReader r;
    r.setFileName(image_source_path);
    r.setQuality(100);
    if(!r.canRead()){
        QFileInfo fi(image_source_path);
        //ASSERT(0, "load-scaled - can't read") << image_source_path;
        qWarning() << "load-scaled - can't read" << image_source_path << fi.size();
        return INVALID_IMG;
    }

    QSize orig = r.size();
    if(orig.width() == 0 || orig.height() == 0){
        ASSERT(0, "load-scaled - zero size image") << image_source_path;
        return INVALID_IMG;
    }


    if (orig.width() > scale2_w || orig.height() > scale2_h) 
        {
            if (orig.width() < orig.height())
                {
                    qreal s = (qreal)orig.height() / scale2_h;
                    qreal w = orig.width() / s;
                    r.setScaledSize(QSize((int)w, scale2_h));
                }
            else
                {
                    qreal s = (qreal)orig.width() / scale2_w;
                    qreal h = orig.height() / s;
                    r.setScaledSize(QSize(scale2_w, (int)h));
                }          
        }

    QImage newImg = r.read();
    return newImg;
};
bool make_media(QString source_image_path, 
                QString source_raw_file_path, 
                QString dest_image_path, 
                QString dest_raw_file_path,
                QString dest_mp1, 
                QString dest_mp2)
{
    if(!QFile::exists(source_image_path)){
        ASSERT(0, "make_media - no-file") << source_image_path;
        return false;
    }

    //..
    QImageReader r;
    r.setFileName(source_image_path);
    r.setQuality(100);///making <100 doesn't make size smaller
    if (!r.canRead()) {
        ASSERT(0, "load-scaled - can't read") << source_image_path;
        return false;
    }
    QImage origImg = r.read();
    if (origImg.isNull()) {
        ASSERT(0, "can't read image") << source_image_path;
        return false;
    }

    if (origImg.width() > MP2_WIDTH ||
        origImg.height() > MP2_HEIGHT) 
    {
        QImage mp2 = origImg.scaled(MP2_WIDTH, MP2_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (mp2.isNull()) {
            ASSERT(0, "can't scale to mp2") << source_image_path;
            return false;
        }
        if (!mp2.save(dest_mp2)) {
            ASSERT(0, "can't save mp2") << dest_mp2 << source_image_path;
            return false;
        }
    }
    else {
        if (!origImg.save(dest_mp2)) {
            ASSERT(0, "can't save mp2") << dest_mp2 << source_image_path;
            return false;
        }
    }

    QImage mp1 = origImg.scaled(MP1_WIDTH, MP1_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (mp1.isNull()) {
        ASSERT(0, "can't scale to mp1") << source_image_path;
        return false;
    }
    if (!mp1.save(dest_mp1)) {
        ASSERT(0, "can't save mp1") << dest_mp1 << source_image_path;
        return false;
    }

    if (!dest_image_path.isEmpty()) {
        if (!copyOverFile(source_image_path, dest_image_path)) {
            ASSERT(0, "failed to copy image") << source_image_path << dest_image_path;
            return false;
        }
    }
    if (!dest_raw_file_path.isEmpty()) {
        if (!copyOverFile(source_raw_file_path, dest_raw_file_path)) {
            ASSERT(0, "failed to copy image raw") << source_raw_file_path << dest_raw_file_path;
            return false;
        }
    }   
    
    return true;
}

::STRING_LIST supported_raw_formats()
{
    ::STRING_LIST rv;
    if (rv.empty()) {
        rv.push_back("cr2");
        rv.push_back("CR2");
        rv.push_back("nef");
        rv.push_back("NEF");
    }
    return rv;
};

