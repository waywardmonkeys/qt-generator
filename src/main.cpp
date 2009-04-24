/****************************************************************************
**
** Copyright (C) 1992-2009 Nokia. All rights reserved.
**
** This file is part of Qt Jambi.
**
** ** $BEGIN_LICENSE$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
** 
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
** 
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
** 
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** 
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $END_LICENSE$

**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "main.h"
#include "asttoxml.h"
#include "reporthandler.h"
#include "typesystem.h"
#include "generatorset.h"
#include "fileout.h"
#include "binder.h"


#include <QDir>

void ReportHandler_message_handler(const std::string &str)
{
    ReportHandler::warning(QString::fromStdString(str));
}


void displayHelp(GeneratorSet *generatorSet);

#include <QDebug>
int main(int argc, char *argv[])
{
    GeneratorSet *gs = GeneratorSet::getInstance();

    QString default_file = "qtjambi_masterinclude.h";
    QString default_system = "build_all.txt";

    QString fileName;
    QString typesystemFileName;
    QString pp_file = ".preprocessed.tmp";
    QStringList rebuild_classes;

    QMap<QString, QString> args;

    int argNum = 0;
    for (int i=1; i<argc; ++i) {
        QString arg(argv[i]);
        arg = arg.trimmed();
        if( arg.startsWith("--") ) {
            int split = arg.indexOf("=");
            if( split > 0 )
                args[arg.mid(2).left(split-2)] = arg.mid(split + 1).trimmed();
            else
                args[arg.mid(2)] = QString();
        } else if( arg.startsWith("-")) {
            args[arg.mid(1)] = QString();
        } else {
            argNum++;
            args[QString("arg-%1").arg(argNum)] = arg;
        }
    }

    if (args.contains("no-suppress-warnings")) {
        TypeDatabase *db = TypeDatabase::instance();
        db->setSuppressWarnings(false);
    }

    if (args.contains("include-eclipse-warnings")) {
        TypeDatabase *db = TypeDatabase::instance();
        db->setIncludeEclipseWarnings(true);
    }

    if (args.contains("debug-level")) {
        QString level = args.value("debug-level");
        if (level == "sparse")
            ReportHandler::setDebugLevel(ReportHandler::SparseDebug);
        else if (level == "medium")
            ReportHandler::setDebugLevel(ReportHandler::MediumDebug);
        else if (level == "full")
            ReportHandler::setDebugLevel(ReportHandler::FullDebug);
    }

    if (args.contains("dummy")) {
        FileOut::dummy = true;
    }

    if (args.contains("diff")) {
        FileOut::diff = true;
    }

    if (args.contains("rebuild-only")) {
        QStringList classes = args.value("rebuild-only").split(",", QString::SkipEmptyParts);
        TypeDatabase::instance()->setRebuildClasses(classes);
    }

    fileName = args.value("arg-1");

    typesystemFileName = args.value("arg-2");
    if (args.contains("arg-3"))
        displayHelp(gs);

    if (fileName.isEmpty())
        fileName = default_file;

    if (typesystemFileName.isEmpty())
        typesystemFileName = default_system;

    if (fileName.isEmpty() || typesystemFileName.isEmpty() )
        displayHelp(gs);

    if (!gs->readParameters(args))
        displayHelp(gs);

    printf("Running the Qt Jambi Generator. Please wait while source files are being generated...\n");

    if (!TypeDatabase::instance()->parseFile(typesystemFileName))
        qFatal("Cannot parse file: '%s'", qPrintable(typesystemFileName));


    if (!Preprocess::preprocess(fileName, pp_file, args.value("include-paths"))) {
        fprintf(stderr, "Preprocessor failed on file: '%s'\n", qPrintable(fileName));
        return 1;
    }

    if (args.contains("ast-to-xml")) {
    astToXML(pp_file);
    return 0;
    }

    Binder::installMessageHandler(ReportHandler_message_handler);
    gs->buildModel(pp_file);
    if (args.contains("dump-object-tree")) {
        gs->dumpObjectTree();
        return 0;
    }
    printf("%s\n", qPrintable(gs->generate()));

    printf("Done, %d warnings (%d known issues)\n", ReportHandler::warningCount(),
           ReportHandler::suppressedCount());
}


void displayHelp(GeneratorSet* generatorSet) {
#if defined(Q_OS_WIN32)
    char path_splitter = ';';
#else
    char path_splitter = ':';
#endif
    printf("Usage:\n  generator [options] header-file typesystem-file\n\n");
    printf("Available options:\n\n");
    printf("General:\n");
    printf("  --debug-level=[sparse|medium|full]        \n"
           "  --dump-object-tree                        \n"
           "  --help, -h or -?                          \n"
           "  --no-suppress-warnings                    \n"
           "  --include-eclipse-warnings                \n"
           "  --output-directory=[dir]                  \n"
           "  --include-paths=<path>[%c<path>%c...]     \n"
           "  --print-stdout                            \n",
           path_splitter, path_splitter);

    printf("%s", qPrintable( generatorSet->usage()));
    exit(0);
}
