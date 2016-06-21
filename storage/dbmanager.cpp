#include "dbmanager.h"
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringList>
#include <QFileInfo>
#include <QVector>



int current= 0;
QStringList down_links;
QString imgpath;
int revision_number = 0;


dbmanager::dbmanager(QObject *parent) : QObject(parent)
{

}





bool del_from_db(QString id,int revid)
{
    bool done;
    QDir databasePath;
    QString path = databasePath.currentPath()+"WTL.db";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(path);
    if(!db.open())
    {
        qDebug() <<"error in opening DB";
    }
    else
    {
        qDebug() <<"connected to DB" ;

    }
    QSqlQuery query;

    query.prepare("DELETE FROM Pages WHERE page_ID = '" + id + "'");



    if(query.exec())
    {
        qDebug() << "deleted from table Pages";
        done = true;

    }
    else
    {
        qDebug() << query.lastError();

    }
    query.prepare("DELETE FROM Dependencies WHERE revision_number = '" + QString::number(revid) + "'");


    if(query.exec())
    {
        qDebug() << "deleted from table Dependencies";
        done = true;

    }
    else
    {
        qDebug() << query.lastError();

    }
    if(done == true)
        return true;
    else
        return false;


}

QString clean_text(QString text)
{
    text = text.replace("\n","");
    text = text.replace("&#39;/index.php", "http://en.wikitolearn.org/index.php");
    text = text.replace("&amp;","&");
    text = text.replace("MathShowImage&amp;", "MathShowImage&");
    text = text.replace("mode=mathml&#39;", "mode=mathml""");
    text = text.replace("<meta class=\"mwe-math-fallback-image-inline\" aria-hidden=\"true\" style=\"background-image: url(" ,"<img style=\"background-repeat: no-repeat; background-size: 100% 100%; vertical-align: -0.838ex;height: 2.843ex;\""   "src=");
    text = text.replace("<meta class=\"mwe-math-fallback-image-display\" aria-hidden=\"true\" style=\"background-image: url(" ,"<img style=\"background-repeat: no-repeat; background-size: 100% 100%; vertical-align: -0.838ex;height: 2.843ex;\""  "src=");
    text = text.replace("&mode=mathml);" , "&mode=mathml\">");
    return(text);
}

bool check_links(QString text)
{

    QRegularExpression link_regex("src=(?<path>.*?)>");
    QRegularExpressionMatch contain = link_regex.match(text);
    qDebug() << contain;

    if(contain.capturedLength() > 0)
    {
        return  true;
    }
    else
    {
        return false;
    }

}



bool add_depend(QString filename , int revision_number)
{
    QDir databasePath;
    QString path = databasePath.currentPath()+"WTL.db";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(path);
    if(!db.open())
    {
        qDebug() <<"error in opening DB";
    }
    else
    {
        qDebug() <<"connected to DB" ;
    }

    QSqlQuery query;

    query.prepare("INSERT INTO Dependencies (depe_fileName,revision_number) "
                  "VALUES (:depe_filename , :revision_number )");



    query.bindValue(":depe_filename",filename);
    query.bindValue(":revision_number", revision_number);



    if(query.exec())
    {
        qDebug() << "done";
        db.close();
        return(true);
    }
    else
    {
        qDebug() << query.lastError();
        db.close();

    }
    return (false);

}




bool add_in_db(int pageid , int revid)
{
    revision_number = revid ;
    QDir databasePath;
    QString path = databasePath.currentPath()+"WTL.db";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(path);
    if(!db.open())
    {
        qDebug() <<"error in opening DB";
    }
    else
    {
        qDebug() <<"connected to DB" ;
    }

    QSqlQuery query;

    query.prepare("INSERT INTO pages (page_ID,page_revision) "
                  "VALUES (? , ?)");
    query.bindValue(0,pageid);
    query.bindValue(1, revid);

    if(query.exec())
    {
        qDebug() << "done";
        return(true);
        db.close();
    }
    else
    {
        qDebug() << query.lastError();
        db.close();

    }
    return (false);
}

bool save_images(QString filename)
{
    QString content , newpath , style;
    qDebug() << filename +" <- html filename ";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"unable to open file";
        return false;
    }
    else{
        content = file.readAll();
        //  download images here

        QRegularExpression link_regex("src=(?<path>.*?)>");
        QRegularExpressionMatchIterator links = link_regex.globalMatch(content);

        while (links.hasNext()) {
            QRegularExpressionMatch match = links.next();
            QString down_link = match.captured(1).remove(QString("&mode=mathml\"")); ;
            //            qDebug()<<down_link.remove(QString("&mode=mathml\""));
            down_links << down_link;  //prepare list of downloads
            //start downloading images
            QString d = content.replace("&mode=mathml\"",".svg"); //clean img src in local html file
            qDebug() << imgpath ;
            newpath = d.replace("http://en.wikitolearn.org/index.php?title=Special:MathShowImage&hash=",""); // clean img src in local html file and prepare the local path those are to be saved in html file

        }



        qDebug() << down_links; //got the list of downloads
        file.close();

    }

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() <<"unable to write to file";
        return false;
    }

    else
    {
        qDebug() <<"write to file here";
        QTextStream out(&file);

        newpath = "<link rel=\"stylesheet\" type=\"text/css\" href=\"main.css\">" + newpath;
        newpath = newpath.replace("svg> background-repeat:", "svg> <!--background-repeat:");
        newpath = newpath.replace("ex;\" />", "-->");


        out << newpath;
        // qDebug()<<newpath;

        file.close();

        // move html file to their respective folder
        QString temp_name = filename;
       QString new_name = temp_name.replace(".html","");
       QString css_path = new_name;
        new_name = new_name + "/" + filename;
        file.rename(filename,new_name);
        css_path = css_path + "/main.css";
        file.copy("main.css",css_path);

    }

    dbmanager *d = new dbmanager(0) ;
    d->doDownload(down_links);

    return true ;

}


//start downloading images




void dbmanager::doDownload(const QVariant& v)
{
    if (v.type() == QVariant::StringList) {


        QNetworkAccessManager *manager= new QNetworkAccessManager(this);

        QUrl url = v.toStringList().at(current);

        filename = url.toString().remove("http://en.wikitolearn.org/index.php?title=Special:MathShowImage&hash=");
        m_network_reply = manager->get(QNetworkRequest(QUrl(url)));

        connect(m_network_reply, SIGNAL(downloadProgress (qint64, qint64)),this, SLOT(updateDownloadProgress(qint64, qint64)));
        connect(m_network_reply,SIGNAL(finished()),this,SLOT(downloadFinished()));


    }
}



void dbmanager::downloadFinished(){
    qDebug()<<filename;
    if(m_network_reply->error() == QNetworkReply::NoError){


        m_file =  new QFile(imgpath+"/"+filename+".svg");
        qDebug()<<imgpath+"/"+filename;
        if(!m_file->open(QIODevice::ReadWrite | QIODevice::Truncate)){
            qDebug() << m_file->errorString();
        }
        m_file->write(m_network_reply->readAll());

        QByteArray bytes = m_network_reply->readAll();
        QString str = QString::fromUtf8(bytes.data(), bytes.size());
        int statusCode = m_network_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << QVariant(statusCode).toString();
    }
    m_file->flush();
    m_file->close();
    int total = down_links.count();
    if(current<total-1){
        current++;
        qDebug()<<"current = "<<current<<"total = "<<total;
        doDownload(down_links);}
    else if(current==total-1)
    {
        qDebug()<<"download complete";
    }

    bool success = false ;
    QString fname = filename;
    success = add_depend(fname,revision_number);
    if(success == true)
    {
        qDebug() <<"added in dependency table";
    }
    else
    {
        qDebug() << " error in adding to dependency table";
    }


}


void dbmanager::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    //    ui->progressBar->setMaximum(totalBytes);
    //    ui->progressBar->setValue(bytesRead);
    qDebug()<<bytesRead<<totalBytes;
}

void save_file(QString text , int pageid , int revid)
{
    if(!check_links(text))
    {
        QDir dir;
        QString Folder_name = QString::number(pageid);
        if(QDir(Folder_name).exists())
        {
            qDebug() << " already exist ";

        }
        else{
            dir.mkdir(Folder_name);


            QString filename = Folder_name+".html";
            QFile file(filename);
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);

            text = "<link rel=\"stylesheet\" type=\"text/css\" href=\"main.css\">" +text;
            out << text;

            // optional, as QFile destructor will already do it:
            file.close();
            // move html file to their respective folder
            QString temp_name = filename;
           QString new_name = temp_name.replace(".html","");
           QString css_path = new_name;
            new_name = new_name + "/" + filename;
            file.rename(filename,new_name);
            css_path = css_path + "/main.css";
            file.copy("main.css",css_path);

            bool success = add_in_db(pageid,revid);
            if(success == true)
            {
                qDebug() <<"entry added to DB successfully ";
            }
            else
            {
                qDebug() <<" failed to add in DB ";
            }

        }
    }

        else {

            QDir dir;
            dbmanager d;
            d.imageDownloadPath = QString::number(pageid);
            imgpath =d.imageDownloadPath;

            if(QDir(d.imageDownloadPath).exists())
            {
                qDebug() << " already exist ";
            }
            else{
                dir.mkdir(d.imageDownloadPath);

                QString filename = d.imageDownloadPath+".html";

                QFile file(filename);
                file.open(QIODevice::WriteOnly | QIODevice::Text);
                QTextStream out(&file);
                out << text;

                // optional, as QFile destructor will already do it:
                file.close();
                bool success = add_in_db(pageid,revid);
                if(success == true)
                {
                    qDebug() <<"entry added to DB successfully ";
                }
                else
                {
                    qDebug() <<" failed to add in DB ";
                }

                success = save_images(filename);

                if(success == true)
                {
                    qDebug() << "images downloaded successfully ";
                }
                else
                {
                    qDebug() << "error in downloading images";
                }


            }

        }
}

void del_file(QString pageid)
{
    QDir dir ;

    if(QDir(pageid).exists())
    {
        dir = pageid;
        dir.removeRecursively();

    }
    else{
        qDebug() << "cannot delete or folder does not exist";

}
}


QString dbmanager::add(QString p_url)
{

    QString text ;
    int pageid , revid;

    QString requested_url = "http://en.wikitolearn.org/api.php?action=parse&page="+p_url+"&format=json";

    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req( requested_url );
    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        //success
        //qDebug() << "Success" <<reply->readAll();
        QString   html = (QString)reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(html.toUtf8());

        QJsonObject jsonObj = jsonResponse.object();


        text = jsonObj["parse"].toObject()["text"].toObject()["*"].toString();
        pageid = jsonObj["parse"].toObject()["pageid"].toInt();
        revid = jsonObj["parse"].toObject()["revid"].toInt();

        //clean the result from the API
        text = clean_text(text);
        //  qDebug() << text;
        qDebug() <<pageid;

        delete reply;
    }

    else {
        //failure
        qDebug() << "Failure" <<reply->errorString();
        delete reply;
    }

    // ******************* here ****************

    save_file(text , pageid , revid);

    // ***************************

    static auto i = 0;
       return QString("%1: %2").arg(++i).arg(p_url);


    }

    QString dbmanager::del(QString pageid)
    {
        qDebug() <<"DELETION CODE GOES HERE";
        del_file(pageid);

        QDir databasePath;
        QString path = databasePath.currentPath()+"WTL.db";
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
        db.setDatabaseName(path);
        if(!db.open())
        {
            qDebug() <<"error in opening DB";
        }
        else
        {
            qDebug() <<"connected to DB" ;

        }
        int revid ;
        QSqlQuery query;

      query.prepare("Select page_revision from Pages where page_ID = :id");
      query.bindValue(":id", pageid);
      query.exec();

      if (query.next()) {
           revid = query.value(0).toInt();

      }
      db.close();

       del_from_db(pageid,revid);

        static auto i = 0;
           return QString("%1: %2").arg(++i).arg(pageid);

    }

    bool check_revision(QString id , int revision_number)
    {
        int pageid;

        QEventLoop eventLoop;

        // "quit()" the event-loop, when the network request "finished()"
        QNetworkAccessManager mg;
        QObject::connect(&mg, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

        // the HTTP request
        QString url = "http://en.wikitolearn.org/api.php?action=parse&pageid="+id+"&format=json";
        QNetworkRequest re( ( url ) );
        QNetworkReply *reply = mg.get(re);
        eventLoop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            //success
            //qDebug() << "Success" <<reply->readAll();
            QString   html = (QString)reply->readAll();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(html.toUtf8());

            QJsonObject jsonObj = jsonResponse.object();

            int  revid = jsonObj["parse"].toObject()["revid"].toInt();
            pageid = jsonObj["parse"].toObject()["pageid"].toInt();
            qDebug() << jsonObj["parse"].toObject()["title"].toString();


            if(revision_number == revid)
            {
                delete reply;
                return true ;
            }
            else
            {
                qDebug() << "update page";
                QString text = jsonObj["parse"].toObject()["text"].toObject()["*"].toString();
                text = clean_text(text);
                bool del = del_from_db(id,revid);
                //check if deletion was successfull
                if(del == true)
                {
                    qDebug() << "deletion from DB done";
                }
                else
                {
                    qDebug() << "error in deletion from DB";
                }
            QString pid = QString::number(pageid);
            del_file(pid);
            save_file( text ,  pageid ,  revision_number);

                
            }

    }


    }



    void dbmanager::update()
    {

        QDir databasePath;
        QString path = databasePath.currentPath()+"WTL.db";
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
        db.setDatabaseName(path);
        if(!db.open())
        {
            qDebug() <<"error in opening DB";
        }
        else
        {
            qDebug() <<"connected to DB" ;
        }


        bool change = false ;
        QSqlQuery count;

        QVector<QString> id ;
        QVector<int> revid;


         QSqlQuery query("SELECT page_ID , page_revision FROM Pages");
           while (query.next()) {
                QString i = query.value(0).toString();
                id.push_back(i);
               int r = query.value(1).toInt();
               revid.push_back(r);

           }
           for(int i = 0 ; i < id.size() ; i++){
           change  = check_revision(id[i] , revid[i]);
               qDebug() << id[i];
               qDebug() << revid[i];
           if(change == true)
           {
               qDebug() << " same";
           }
           else
           {
               qDebug() << "need update";

           }
        }

    }

