#include <iostream>

#include <QDir>
#include <QDirIterator>
#include <QStringList>
#include <QFileInfoList>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QVariant>

#include <QtDebug>

#include <grantlee_core.h>
#include "grantlee_paths.h"

#include "ClassTemplates.h"

#define TEMPALTE_DIR "templates"
#define SOURCE_DIR "sources"
#define API_DIR "API_generated"

using namespace std;

void getTemplateParents( const QString& tmplPath, QVariantList& parentList )
{
  //TODO
  //Get the templateDir (templates root)
  //search recursively for tmplParentPath
  QFile tmpl( tmplPath );
  tmpl.open( QIODevice::ReadOnly|QIODevice::Text );
  QString text = tmpl.readAll();

  QRegExp extends("{%\\s+extends\\s+\"(\\w+)\"\\s+%}");

  if (extends.indexIn( text ) != -1) {
    QString tmplParentName = extends.cap(1);
    parentList << tmplParentName;
    getTemplateParents( tmplParentPath, parentList );
  else
    return;
}

typedef Grantlee::FileSystemTemplateLoader GFSLoader;

Grantlee::Engine* getEngine( const QStringList& tmplDirs )
{
  QStringList allTmplDirs( tmplDirs );

  //Get all template subdirectories
  foreach (QString dir, tmplDirs) {
    QDirIterator subdir( dir,
                         QDir::AllDirs|QDir::NoDotAndDotDot|QDir::Readable|QDir::Executable,
                         QDirIterator::Subdirectories );
    while (subdir.hasNext()) {
      subdir.next();
      allTmplDirs << dir + "/" + subdir.fileName();
    }
  }
  qDebug() << "Loading templates from:";
  qDebug() << allTmplDirs;

  Grantlee::Engine *engine = new Grantlee::Engine();
  GFSLoader::Ptr loader = GFSLoader::Ptr( new GFSLoader() );
  loader->setTemplateDirs( allTmplDirs );

  engine->addTemplateLoader( loader );
  engine->setPluginPaths( QStringList() << "." << GRANTLEE_PLUGIN_PATH );
  return engine;
}

const QStringList templateSuffixes(
    QStringList() <<
    "*.template.h" <<
    "*.template.c" <<
    "*.template.cc" <<
    "*.template.txt"
);

QDir templateDir, sourceDir, outputDir;

int main(int argc, char *argv[])
{
  if (QString(argv[1]) == "-h" ||
      QString(argv[1]) == "--help") {
    cout << "Usage: " << argv[0] << " [template dir]" << " [sources dir]" << " [output dir]" << endl;
    return 0;
  }
  templateDir.setPath( argc > 1 ? argv[1] : TEMPALTE_DIR );
  sourceDir.setPath  ( argc > 2 ? argv[2] : SOURCE_DIR   );
  outputDir.setPath  ( argc > 3 ? argv[3] : API_DIR      );

  if (!templateDir.exists()) {
    qCritical() << "Directory" << templateDir.path() << "does not exist";
    return 1;
  }
  if (!sourceDir.exists()) {
    qCritical() << "Directory" << sourceDir.path() << "does not exist";
    return 1;
  }
  if (!outputDir.exists()) {
    qDebug() << "Creating directory" << outputDir.path();
    QDir currentDir;
    if (!currentDir.mkdir(outputDir.path())) {
      qCritical() << "Directory" << outputDir.path() << "cannot be created";
      return 1;
    }
  }

  //Check for newly added classes and create missing templates
  ClassTemplates tpl( sourceDir.path(), templateDir.path());
  tpl.updateTemplates = true;
  tpl.createTemplates();
  tpl.createSources();
  QVariantList classes = tpl.getAllClasses();
  QVariant classinfo;

  //Setup grantlee
  Grantlee::Engine *engine = getEngine( QStringList() << templateDir.path() << sourceDir.path() );

  QDirIterator templateFile( templateDir.path(),
               templateSuffixes,
               QDir::Files|QDir::Readable,
               QDirIterator::Subdirectories );

  while (templateFile.hasNext()) {
    templateFile.next();
    QFileInfo templateFileInfo = templateFile.fileInfo();

    Grantlee::Template t = engine->loadByName( templateFileInfo.fileName() );
    Grantlee::Context c;

    QString class_base_name = templateFileInfo.baseName();
    int idx = class_base_name.indexOf("_");
    class_base_name.truncate(idx);

    QString sourceName, oy;
    if (class_base_name == "Struct") {
      classinfo = tpl.getStructClass();
      oy = "oy";
    } else {
      int i;
      for (i=0; i<classes.size(); i++) {
        if (class_base_name == classes.at( i ).value<QObject*>()->property("baseName").toString()) {
          classinfo = classes.at( i );
          oy = "oy";
          break;
        }
      }
      if (i == classes.size())
        qDebug() << "No class found for" << templateFile.fileName();
    }
    sourceName = oy + templateFileInfo.baseName() + "." + templateFileInfo.suffix();
    QFile sourceFile( outputDir.filePath( sourceName ) );
    QFileInfo sourceFileInfo(sourceFile);

    // Get the template parent list
    QVariantList parents;
    parents << templateFileInfo.fileName();
    getTemplateParents( templateFileInfo.filePath(), parents );

    c.insert( "file_name", sourceName );
    c.insert( "classes", classes );
    c.insert( "class", classinfo );
    c.insert( "parents", parents);

    QString newFileContents = t->render( &c );

    //1. There is no source file yet
    if (not sourceFile.exists()) {
      sourceFile.open(QIODevice::WriteOnly | QIODevice::Text);
      sourceFile.write( newFileContents.toUtf8() );
      clog << "Creating " << sourceFile.fileName().toUtf8().data() << endl;
    } else
    //2. The template file is more recent
    if (templateFileInfo.lastModified() > sourceFileInfo.lastModified()) {
      sourceFile.open(QIODevice::WriteOnly | QIODevice::Text);
      sourceFile.write( newFileContents.toUtf8() );
      clog << "Updating " << sourceFile.fileName().toUtf8().data() << endl;
    } else {
      sourceFile.open(QIODevice::ReadOnly | QIODevice::Text);
      QString oldFileContents = sourceFile.readAll();
      //4. The souce file has changed //TODO
      if (oldFileContents != newFileContents)
        cerr << "Warning: " << sourceFile.fileName().toUtf8().data() << " has changed!" << endl;
      else
      //3. There is no difference in file contents -> do nothing
        clog << "Skipping " << sourceFile.fileName().toUtf8().data() << endl;
    }

    //5. Both have changed //TODO
  }
  delete engine;

  return 0;
}
