#include "asttoxml.h"
#include "control.h"
#include "parser.h"
#include "binder.h"
#include "codemodel.h"

#include <QXmlStreamWriter>
#include <QTextStream>
#include <QTextCodec>
#include <QFile>

void astToXML(QString name) {
    QFile file(name);

    if (!file.open(QFile::ReadOnly))
        return;

    QTextStream stream(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));
    QByteArray contents = stream.readAll().toUtf8();
    file.close();

    Control control;
    Parser p(&control);
    pool __pool;

    TranslationUnitAST *ast = p.parse(contents, contents.size(), &__pool);

    CodeModel model;
    Binder binder(&model, p.location());
    FileModelItem dom = binder.run(ast);

    QFile outputFile;
    if (!outputFile.open(stdout, QIODevice::WriteOnly))
	{
	    return;
	}

    QXmlStreamWriter s( &outputFile);
    s.setAutoFormatting( true );

    s.writeStartElement("code");

    QHash<QString, NamespaceModelItem> namespaceMap = dom->namespaceMap();
    foreach (NamespaceModelItem item, namespaceMap.values()) {
	s.writeStartElement("namespace");
	s.writeCharacters(item->name());
	s.writeEndElement();
    }
    
    QHash<QString, ClassModelItem> typeMap = dom->classMap();
    foreach (ClassModelItem item, typeMap.values()) {
        QString qualified_name = item->qualifiedName().join("::");
	s.writeStartElement("class");
	s.writeAttribute("name", qualified_name);
	
	/*	QHash<QString, ClassModelItem> typeMap = item->classMap();
	foreach (ClassModelItem item, typeMap.values()) {
	    QString qualified_name = item->qualifiedName().join("::");

	    s << "   class: " << qualified_name << endl;
	}*/

	QHash<QString, EnumModelItem> enumMap = item->enumMap();
	foreach (EnumModelItem item, enumMap.values()) {
	    QString qualified_name = item->qualifiedName().join("::");
	    s.writeStartElement("enum");
	    s.writeAttribute("name", qualified_name);
	   
	    EnumeratorList enumList = item->enumerators();
	    for(int i=0; i < enumList.size() ; i++) {
                s.writeStartElement("enumerator");
		if( !enumList[i]->value().isEmpty() )
		    s.writeAttribute("value", enumList[i]->value());
		s.writeCharacters(enumList[i]->name());
        
		s.writeEndElement();
	    }
	    s.writeEndElement();
	}

	QHash<QString, FunctionModelItem> functionMap = item->functionMap();
	foreach (FunctionModelItem item, functionMap.values()) {
	    QString qualified_name = item->qualifiedName().join("::");
	    s.writeStartElement("function");
	    s.writeAttribute("name", qualified_name);

	    ArgumentList arguments = item->arguments();
	    for(int i=0; i < arguments.size() ; i++) {
		s.writeStartElement("argument");
		s.writeAttribute("type",  arguments[i]->type().qualifiedName().join("::"));
		s.writeEndElement();
	    }

	    s.writeEndElement();
	}
	s.writeEndElement();
    }
    s.writeEndElement();
}
