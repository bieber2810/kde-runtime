#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <qobject.h>
#include <qptrlist.h>
#include <qstring.h>

#include <kpixmap.h>
#include <kio/job.h>

#include "docentrytraverser.h"

class QWidget;
class KProcess;
class KConfig;
class KHTMLPart;

namespace KHC {

class Formatter;
class SearchEngine;
class View;
class SearchHandler;

class SearchTraverser : public QObject, public DocEntryTraverser
{
    Q_OBJECT
  public:
    SearchTraverser( SearchEngine *engine, int level );
    ~SearchTraverser();

    void process( DocEntry * );
    
    void startProcess( DocEntry * );

    DocEntryTraverser *createChild( DocEntry * );

    DocEntryTraverser *parentTraverser();

    void deleteTraverser();

    void finishTraversal();

  protected slots:
    void showSearchResult( const QString &result );
    void showSearchError( const QString &error );

  private:
    SearchEngine *mEngine;
    int mLevel;

    DocEntry *mEntry;
    QString mJobData;
    
    QString mResult;
};


class SearchEngine : public QObject
{
    Q_OBJECT
  public:
    enum Operation { And, Or };

    SearchEngine( View * );
    ~SearchEngine();

    bool initSearchHandlers();

    bool search( QString words, QString method = "and", int matches = 5,
                 QString scope = "" );

    Formatter *formatter() const; 
    View *view() const;

    QString substituteSearchQuery( const QString &query );

    static QString substituteSearchQuery( const QString &query,
      const QString &identifier, const QStringList &words, int maxResults,
      Operation operation );

    void finishSearch();

    QString errorLog() const;

    bool isRunning() const;

    SearchHandler *handler( const QString &documentType ) const;

    QStringList words() const;
    int maxResults() const;
    Operation operation() const;

  signals:
    void searchFinished();

  protected slots:
    void searchStdout(KProcess *proc, char *buffer, int buflen);
    void searchStderr(KProcess *proc, char *buffer, int buflen);
    void searchExited(KProcess *proc);

  protected:
    void processSearchQueue();
    
  private:
    KProcess *mProc;
    bool mSearchRunning;
    QString mSearchResult;

    QString mStderr;

    View *mView;
    
    QString mWords;
    int mMatches;
    QString mMethod;
    QString mLang;
    QString mScope;

    QStringList mWordList;
    int mMaxResults;
    Operation mOperation;
    
    DocEntryTraverser *mRootTraverser;

    QMap<QString, SearchHandler *> mHandlers;
};

}

#endif
// vim:ts=2:sw=2:et
