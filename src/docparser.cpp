#include "docparser.h"

#include "metajava.h"
#include "reporthandler.h"

#include <QtCore/QFileInfo>
#include <QtCore/QFile>

#include <QtXml>

DocParser::DocParser(const QString &name)
    : m_doc_file(name),
      m_dom(0)
{
    build();
}

DocParser::~DocParser()
{
    delete m_dom;
}

QString DocParser::documentation(const MetaJavaClass *meta_class) const
{
    if (!m_dom)
        return QString();

    QDomElement root_node = m_dom->documentElement();

    QString class_name = root_node.attribute("name");
    QString doc = root_node.attribute("doc");

    if (class_name != meta_class->name()) {
        ReportHandler::warning("Documentelement file contains unexpected class: " + class_name + ", file=" + m_doc_file);
    }

    return doc;
}

QString DocParser::documentationForFunction(const QString &signature, const QString &tag) const
{
    if (!m_dom)
        return QString();

    QDomElement root_node = m_dom->documentElement();
    QDomNodeList functions = root_node.elementsByTagName(tag);

    for (int i=0; i<functions.size(); ++i) {
        QDomNode node = functions.item(i);

        QDomElement *e = (QDomElement *) &node;

        Q_ASSERT(e->isElement());

        if (e->attribute("name") == signature)
            return e->attribute("doc");
    }

    return QString();
}


QString DocParser::documentationForSignal(const QString &signature) const
{
    return documentationForFunction(signature, "signal");
}

QString DocParser::documentationForFunction(const QString &signature) const
{
    return documentationForFunction(signature, "method");
}

QString DocParser::documentation(const MetaJavaEnum *java_enum) const
{
    if (!m_dom)
        return QString();

    QDomElement root_node = m_dom->documentElement();

    QDomNodeList enums = root_node.elementsByTagName("enum");

    for (int i=0; i<enums.size(); ++i) {
        QDomNode node = enums.item(i);
        QDomElement *e = (QDomElement *) &node;

        Q_ASSERT(e->isElement());

        if (e->attribute("name") == java_enum->name()) {
            return e->attribute("doc");
        }
    }

    return QString();
}


QString DocParser::documentation(const MetaJavaEnumValue *java_enum_value) const
{
    if (!m_dom)
        return QString();

    QDomElement root_node = m_dom->documentElement();

    QDomNodeList enums = root_node.elementsByTagName("enum");

    for (int i=0; i<enums.size(); ++i) {
        QDomNode node = enums.item(i);
        QDomElement *e = (QDomElement *) &node;
        Q_ASSERT(e->isElement());

        QDomNodeList enumValues = e->elementsByTagName("enum-value");
        for (int j=0; j<enumValues.size(); ++j) {
            QDomNode node = enumValues.item(j);
            QDomElement *ev = (QDomElement *) &node;
            if (ev->attribute("name") == java_enum_value->name()) {
                return ev->attribute("doc");
            }
        }
    }

    return QString();
}



void DocParser::build()
{
    if (!QFileInfo(m_doc_file).exists()) {
        ReportHandler::warning("Missing documentation file: " + m_doc_file);
        return;
    }

    QFile f(m_doc_file);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        ReportHandler::warning("Failed to open documentation file: " + m_doc_file);
        return;
    }

    m_dom = new QDomDocument();

    QString error;
    int line, column;

    if (!m_dom->setContent(&f, &error, &line, &column)) {
         ReportHandler::warning(QString("Failed to parse the documentation file:"
                                        " '%1' %2 line=%3 column=%4")
                                .arg(m_doc_file)
                                .arg(error)
                                .arg(line)
                                .arg(column));

        delete m_dom;
        m_dom = 0;

        return;
    }


}