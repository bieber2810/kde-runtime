/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "resourcemerger.h"
#include "datamanagementmodel.h"
#include "classandpropertytree.h"
#include "nepomuktools.h"

#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/XMLSchema>

#include <Soprano/StatementIterator>
#include <Soprano/QueryResultIterator>
#include <Soprano/FilterModel>
#include <Soprano/NodeIterator>
#include <Soprano/LiteralValue>
#include <Soprano/Node>

#include <KDebug>
#include <Soprano/Graph>

using namespace Soprano::Vocabulary;


Nepomuk::ResourceMerger::ResourceMerger(Nepomuk::DataManagementModel* model, const QString& app,
                                        const QHash< QUrl, QVariant >& additionalMetadata,
                                        const StoreResourcesFlags& flags )
{
    m_app = app;
    m_additionalMetadata = additionalMetadata;
    m_model = model;
    m_flags = flags;

    //setModel( m_model );

    // Resource Metadata
    metadataProperties.reserve( 4 );
    metadataProperties.insert( NAO::lastModified() );
    metadataProperties.insert( NAO::userVisible() );
    metadataProperties.insert( NAO::created() );
    metadataProperties.insert( NAO::creator() );
}


Nepomuk::ResourceMerger::~ResourceMerger()
{
}

// void Nepomuk::ResourceMerger::setModel(Soprano::Model* model)
// {
//     Q_ASSERT( model != 0 );
//     m_model = model;
// }

Soprano::Model* Nepomuk::ResourceMerger::model() const
{
    return m_model;
}

void Nepomuk::ResourceMerger::setMappings(const QHash< KUrl, KUrl >& mappings)
{
    m_mappings = mappings;
}

QHash< KUrl, KUrl > Nepomuk::ResourceMerger::mappings() const
{
    return m_mappings;
}

void Nepomuk::ResourceMerger::setAdditionalGraphMetadata(const QHash<QUrl, QVariant>& additionalMetadata)
{
    m_additionalMetadata = additionalMetadata;
}

QHash< QUrl, QVariant > Nepomuk::ResourceMerger::additionalMetadata() const
{
    return m_additionalMetadata;
}

Soprano::Statement Nepomuk::ResourceMerger::resolveStatement(const Soprano::Statement& st)
{
    if( !st.isValid() ) {
        QString error = QString::fromLatin1("Invalid statement encountered");
        setError( error, Soprano::Error::ErrorInvalidStatement );
        return Soprano::Statement();
    }

    Soprano::Node resolvedSubject = resolveMappedNode( st.subject() );
    if( lastError() )
        return Soprano::Statement();

    Soprano::Statement newSt( st );
    newSt.setSubject( resolvedSubject );

    Soprano::Node object = st.object();
    if( ( object.isResource() && object.uri().scheme() == QLatin1String("nepomuk") ) || object.isBlank() ) {
        Soprano::Node resolvedObject = resolveMappedNode( object );
        if( lastError() )
            return Soprano::Statement();
        newSt.setObject( resolvedObject );
    }

    return newSt;
}


QUrl Nepomuk::ResourceMerger::graph()
{
    if( !m_graph.isValid() ) {
        m_graph = createGraph();
        if( !m_graph.isValid() ) {
            setError( QString::fromLatin1("Graph creation failed. A valid graph was not returned %1")
            .arg( m_graph.toString() ), Soprano::Error::ErrorInvalidArgument );
        }
    }
    return m_graph;
}


bool Nepomuk::ResourceMerger::push(const Soprano::Statement& st)
{
    ClassAndPropertyTree *tree = ClassAndPropertyTree::self();
    if( tree->maxCardinality(  st.predicate().uri() ) == 1 ) {
        const bool lazy = ( m_flags & LazyCardinalities );
        const bool overwrite = (m_flags & OverwriteProperties) &&
        tree->maxCardinality( st.predicate().uri() ) == 1;

        if( lazy || overwrite ) {
            // FIXME: This may create some empty graphs
            // Store them somewhere and remove them if they are now empty
            m_model->removeAllStatements( st.subject(), st.predicate(), Soprano::Node() );
        }
    }

    Soprano::Statement statement( st );
    if( statement.context().isEmpty() )
        statement.setContext( m_graph );

    // FIXME: Use the model directly?
    return addStatement( statement ) == Soprano::Error::ErrorNone;
}

Soprano::Error::ErrorCode Nepomuk::ResourceMerger::addStatement(const Soprano::Node& subject, const Soprano::Node& property, const Soprano::Node& object, const Soprano::Node& graph)
{
    return addStatement( Soprano::Statement(subject, property, object, graph) );
}

QUrl Nepomuk::ResourceMerger::createGraph()
{
    return m_model->createGraph( m_app, m_additionalMetadata );
}

QMultiHash< QUrl, Soprano::Node > Nepomuk::ResourceMerger::getPropertyHashForGraph(const QUrl& graph) const
{
    // trueg: this is more a hack than anything else: exclude the inference types
    // a real solution would either ignore supertypes of nrl:Graph in checkGraphMetadata()
    // or only check the new metadata for consistency
    Soprano::QueryResultIterator it
            = model()->executeQuery(QString::fromLatin1("select ?p ?o where { graph ?g { %1 ?p ?o . } . FILTER(?g!=<urn:crappyinference2:inferredtriples>) . }")
                                    .arg(Soprano::Node::resourceToN3(graph)),
                                    Soprano::Query::QueryLanguageSparql);
    //Convert to prop hash
    QMultiHash<QUrl, Soprano::Node> propHash;
    while(it.next()) {
        propHash.insert( it["p"].uri(), it["o"] );
    }
    return propHash;
}


bool Nepomuk::ResourceMerger::areEqual(const QMultiHash<QUrl, Soprano::Node>& oldPropHash,
                                       const QMultiHash<QUrl, Soprano::Node>& newPropHash)
{
    //
    // When checking if two graphs are equal, certain stuff needs to be considered
    //
    // 1. The nao:created might not be the same
    // 2. One graph may contain more rdf:types than the other, but still be the same
    // 3. The newPropHash does not contain the nao:maintainedBy statement

    QSet<QUrl> oldTypes;
    QSet<QUrl> newTypes;

    QHash< QUrl, Soprano::Node >::const_iterator it = oldPropHash.constBegin();
    for( ; it != oldPropHash.constEnd(); it++ ) {
        const QUrl & propUri = it.key();
        if( propUri == NAO::maintainedBy() || propUri == NAO::created() )
            continue;

        if( propUri == RDF::type() ) {
            oldTypes << it.value().uri();
            continue;
        }

        //kDebug() << " --> " << it.key() << " " << it.value();
        if( !newPropHash.contains( it.key(), it.value() ) ) {
            //kDebug() << "False value : " << newPropHash.value( it.key() );
            return false;
        }
    }

    it = newPropHash.constBegin();
    for( ; it != newPropHash.constEnd(); it++ ) {
        const QUrl & propUri = it.key();
        if( propUri == NAO::maintainedBy() || propUri == NAO::created() )
            continue;

        if( propUri == RDF::type() ) {
            newTypes << it.value().uri();
            continue;
        }

        //kDebug() << " --> " << it.key() << " " << it.value();
        if( !oldPropHash.contains( it.key(), it.value() ) ) {
            //kDebug() << "False value : " << oldPropHash.value( it.key() );
            return false;
        }
    }

    //
    // Check the types
    //
    newTypes << NRL::InstanceBase();
    if( !containsAllTypes( oldTypes, newTypes ) || !containsAllTypes( newTypes, oldTypes ) )
        return false;

    // Check nao:maintainedBy
    it = oldPropHash.find( NAO::maintainedBy() );
    if( it == oldPropHash.constEnd() )
        return false;

    if( it.value().uri() != m_model->findApplicationResource(m_app, false) )
        return false;

    return true;
}

bool Nepomuk::ResourceMerger::containsAllTypes(const QSet< QUrl >& types, const QSet< QUrl >& masterTypes)
{
    ClassAndPropertyTree* tree = m_model->classAndPropertyTree();
    foreach( const QUrl & type, types ) {
        if( !masterTypes.contains( type) ) {
            QSet<QUrl> superTypes = tree->allParents( type );
            superTypes.intersect(masterTypes);
            if(superTypes.isEmpty()) {
                return false;
            }
        }
    }

    return true;
}


// Graph Merge rules
// 1. If old graph is of type discardable and new is non-discardable
//    -> Then update the graph
// 2. Otherwsie
//    -> Keep the old graph

bool Nepomuk::ResourceMerger::mergeGraphs(const QUrl& oldGraph)
{
    //
    // Check if mergeGraphs has already been called for oldGraph
    //
    if(m_graphHash.contains(oldGraph)) {
        //kDebug() << "Already merged once, just returning";
        return true;
    }

    QMultiHash<QUrl, Soprano::Node> oldPropHash = getPropertyHashForGraph( oldGraph );
    QMultiHash<QUrl, Soprano::Node> newPropHash = m_additionalMetadataHash;

    // Compare the old and new property hash
    // If both have the same properties then there is no point in creating a new graph.
    // vHanda: This check is very expensive. Is it worth it?
    if( areEqual( oldPropHash, newPropHash ) ) {
        //kDebug() << "SAME!!";
        // They are the same - Don't do anything
        m_graphHash.insert( oldGraph, QUrl() );
        return true;
    }

    QMultiHash<QUrl, Soprano::Node> finalPropHash;
    //
    // Graph type nrl:DiscardableInstanceBase is a special case.
    // Only If both the old and new graph contain nrl:DiscardableInstanceBase then
    // will the new graph also be discardable.
    //
    if( oldPropHash.contains( RDF::type(), NRL::DiscardableInstanceBase() ) &&
        newPropHash.contains( RDF::type(), NRL::DiscardableInstanceBase() ) )
        finalPropHash.insert( RDF::type(), NRL::DiscardableInstanceBase() );

    oldPropHash.remove( RDF::type(), NRL::DiscardableInstanceBase() );
    newPropHash.remove( RDF::type(), NRL::DiscardableInstanceBase() );

    finalPropHash.unite( oldPropHash );
    finalPropHash.unite( newPropHash );

    if( !checkGraphMetadata( finalPropHash ) ) {
        kDebug() << "Graph metadata check FAILED!";
        return false;
    }

    // Add app uri
    if( m_appUri.isEmpty() )
        m_appUri = m_model->findApplicationResource( m_app );
    if( !finalPropHash.contains( NAO::maintainedBy(), m_appUri ) )
        finalPropHash.insert( NAO::maintainedBy(), m_appUri );

    //kDebug() << "Creating : " << finalPropHash;
    QUrl graph = m_model->createGraph( m_app, finalPropHash );

    m_graphHash.insert( oldGraph, graph );
    return true;
}

QMultiHash< QUrl, Soprano::Node > Nepomuk::ResourceMerger::toNodeHash(const QHash< QUrl, QVariant >& hash)
{
    QMultiHash<QUrl, Soprano::Node> propHash;
    ClassAndPropertyTree *tree = ClassAndPropertyTree::self();

    QHash< QUrl, QVariant >::const_iterator it = hash.constBegin();
    QHash< QUrl, QVariant >::const_iterator constEnd = hash.constEnd();
    for( ; it != constEnd; ++it ) {
        Soprano::Node n = tree->variantToNode( it.value(), it.key() );
        if( tree->lastError() ) {
            setError( tree->lastError().message() ,tree->lastError().code() );
            return QMultiHash< QUrl, Soprano::Node >();
        }

        propHash.insert( it.key(), n );
    }

    return propHash;
}

bool Nepomuk::ResourceMerger::checkGraphMetadata(const QMultiHash< QUrl, Soprano::Node >& hash)
{
    ClassAndPropertyTree* tree = m_model->classAndPropertyTree();

    QList<QUrl> types;
    QHash<QUrl, int> propCardinality;

    QHash< QUrl, Soprano::Node >::const_iterator it = hash.constBegin();
    for( ; it != hash.constEnd(); it++ ) {
        const QUrl& propUri = it.key();
        if( propUri == RDF::type() ) {
            Soprano::Node object = it.value();
            if( !object.isResource() ) {
                setError(QString::fromLatin1("rdf:type has resource range. '%1' does not have a resource type.").arg(object.toN3()), Soprano::Error::ErrorInvalidArgument);
                return false;
            }

            // All the types should be a sub-type of nrl:Graph
            // FIXME: there could be multiple types in the old graph from inferencing. all superclasses of nrl:Graph. However, it would still be valid.
            if( !tree->isChildOf( object.uri(), NRL::Graph() ) ) {
                setError( QString::fromLatin1("Any rdf:type specified in the additional metadata should be a subclass of nrl:Graph. '%1' is not.").arg(object.uri().toString()),
                                   Soprano::Error::ErrorInvalidArgument );
                return false;
            }
            types << object.uri();
        }

        // Save the cardinality of each property
        QHash< QUrl, int >::iterator propIter = propCardinality.find( propUri );
        if( propIter == propCardinality.end() ) {
            propCardinality.insert( propUri, 1 );
        }
        else {
            propIter.value()++;
        }
    }

    it = hash.constBegin();
    for( ; it != hash.constEnd(); it++ ) {
        const QUrl & propUri = it.key();
        // Check the cardinality
        int maxCardinality = tree->maxCardinality( propUri );
        int curCardinality = propCardinality.value( propUri );

        if( maxCardinality != 0 ) {
            if( curCardinality > maxCardinality ) {
                setError( QString::fromLatin1("%1 has a max cardinality of %2").arg(propUri.toString()).arg(maxCardinality), Soprano::Error::ErrorInvalidArgument );
                return false;
            }
        }

        //
        // Check the domain and range
        const QUrl domain = tree->propertyDomain( propUri );
        const QUrl range = tree->propertyRange( propUri );

        // domain
        if( !domain.isEmpty() && !tree->isChildOf( types, domain ) ) {
            setError( QString::fromLatin1("%1 has a rdfs:domain of %2").arg( propUri.toString(), domain.toString() ), Soprano::Error::ErrorInvalidArgument);
            return false;
        }

        // range
        if( !range.isEmpty() ) {
            const Soprano::Node& object = it.value();
            if( object.isResource() ) {
                if( !isOfType( object.uri(), range ) ) {
                    setError( QString::fromLatin1("%1 has a rdfs:range of %2").arg( propUri.toString(), range.toString() ), Soprano::Error::ErrorInvalidArgument);
                    return false;
                }
            }
            else if( object.isLiteral() ) {
                const Soprano::LiteralValue lv = object.literal();
                if( lv.dataTypeUri() != range ) {
                    setError( QString::fromLatin1("%1 has a rdfs:range of %2").arg( propUri.toString(), range.toString() ), Soprano::Error::ErrorInvalidArgument);
                    return false;
                }
            }
        } // range
    }

    //kDebug() << hash;
    return true;
}

QUrl Nepomuk::ResourceMerger::createResourceUri()
{
    return m_model->createUri( DataManagementModel::ResourceUri );
}

QUrl Nepomuk::ResourceMerger::createGraphUri()
{
    return m_model->createUri( DataManagementModel::GraphUri );
}

QList< QUrl > Nepomuk::ResourceMerger::existingTypes(const QUrl& uri) const
{
    QList<QUrl> types;
    QList<Soprano::Node> existingTypes = m_model->listStatements( uri, RDF::type(), Soprano::Node() )
                                                  .iterateObjects().allNodes();
    foreach( const Soprano::Node & n, existingTypes ) {
        types << n.uri();
    }
    // all resources have rdfs:Resource type by default
    types << RDFS::Resource();

    return types;
}

bool Nepomuk::ResourceMerger::isOfType(const Soprano::Node & node, const QUrl& type, const QList<QUrl> & newTypes) const
{
    //kDebug() << "Checking " << node << " for type " << type;
    ClassAndPropertyTree * tree = m_model->classAndPropertyTree();

    QList<QUrl> types( newTypes );
    if( !node.isBlank() ) {
        types << existingTypes( node.uri() );
    }
    types += newTypes;

    if( types.isEmpty() ) {
        kDebug() << node << " does not have a type!!";
        return false;
    }

    foreach( const QUrl & uri, types ) {
        if( uri == type || tree->isChildOf( uri, type ) ) {
            return true;
        }
    }

    return false;
}

Soprano::Node Nepomuk::ResourceMerger::resolveMappedNode(const Soprano::Node& node)
{
    // Find in mappings
    const QUrl uri = node.isBlank() ? node.toN3() : node.uri();
    QHash< KUrl, KUrl >::const_iterator it = m_mappings.constFind( uri );
    if( it != m_mappings.constEnd() ) {
        return it.value();
    }

    // Do not resolve the blank nodes which need to be created
    if( node.isBlank() )
        return node;

    // If it is a nepomuk:/ uri, just add it as it is.
    // FIXME: What if this uri doesn't exist?
    if( uri.scheme() == QLatin1String("nepomuk") ) {
        return node;
    }

    return node;

    //This should never happen
    //QString error = QString::fromLatin1("Could not resolve ").arg( node.toN3() );
    //setError( error, Soprano::Error::ErrorInvalidStatement );
    //return Soprano::Node();
}

Soprano::Node Nepomuk::ResourceMerger::resolveUnmappedNode(const Soprano::Node& node)
{
    if( !node.isBlank() )
        return node;

    QHash< KUrl, KUrl >::const_iterator it = m_mappings.constFind( QUrl(node.toN3()) );
    if( it != m_mappings.constEnd() ) {
        return it.value();
    }

    //TODO: Make sure the resource gets its metadata -> nao:created, nao:lastModified
    // This is currently done by adding extra nao:create and nao:lastModified statements
    // in storeResources, before calling merge
    QUrl newUri = createResourceUri();
    m_mappings.insert( QUrl(node.toN3()), newUri );
    return newUri;
}

void Nepomuk::ResourceMerger::resolveBlankNodesInList(QList<Soprano::Statement> *stList)
{
    QMutableListIterator<Soprano::Statement> iter( *stList );
    while( iter.hasNext() ) {
        Soprano::Statement &st = iter.next();

        st.setSubject( resolveUnmappedNode(st.subject()) );
        st.setObject( resolveUnmappedNode(st.object()) );
    }
}


namespace {
    QUrl getBlankOrResourceUri( const Soprano::Node & n ) {
        if( n.isResource() ) {
            return n.uri();
        }
        else if( n.isBlank() ) {
            return QString( QLatin1String("_:") + n.identifier() );
        }
        return QUrl();
    }
}

namespace {
    QUrl xsdDuration() {
        return QUrl( Soprano::Vocabulary::XMLSchema::xsdNamespace().toString() + QLatin1String("duration") );
    }
}


bool Nepomuk::ResourceMerger::merge( const Soprano::Graph& stGraph )
{
    //
    // Check if the additional metadata is valid
    //
    QMultiHash<QUrl, Soprano::Node> additionalMetadata = toNodeHash(m_additionalMetadata);
    if( lastError() )
        return false;

    if( !checkGraphMetadata( additionalMetadata ) ) {
        return false;
    }

    //
    // Resolve all the mapped statements
    //
    // FIXME: Use toSet() once 4.7 has released. toSet() is faster, but it requires a newer version
    //        of Soprano
    QList<Soprano::Statement> statements = stGraph.toList();
    QMutableListIterator<Soprano::Statement> sit( statements );
    while( sit.hasNext() ) {
        Soprano::Statement &st = sit.next();
        st = resolveStatement( st );
        if( lastError() )
            return false;
    }

    //
    // Check the statement metadata
    //
    QMultiHash<QUrl, QUrl> types;
    QHash<QPair<QUrl,QUrl>, int> cardinality;

    //
    // First separate all the statements predicate rdf:type.
    // and collect info required to check the types and cardinality
    //
    QList<Soprano::Statement> remainingStatements;
    QList<Soprano::Statement> typeStatements;
    QList<Soprano::Statement> metadataStatements;

    foreach( const Soprano::Statement & st, statements ) {
        const QUrl subUri = getBlankOrResourceUri( st.subject() );
        const QUrl objUri = getBlankOrResourceUri( st.object() );

        const QUrl prop = st.predicate().uri();
        if( prop == RDF::type() ) {
            typeStatements << st;
            types.insert( subUri, objUri );
            continue;
        }
        // we ignore the metadata properties as they will get special
        // treatment duing the merging
        else if( metadataProperties.contains( prop ) ) {
            metadataStatements << st;
            continue;
        }
        else {
            remainingStatements << st;
        }

        // Get the cardinality
        QPair<QUrl,QUrl> subPredPair( subUri, st.predicate().uri() );

        QHash< QPair< QUrl, QUrl >, int >::iterator it = cardinality.find( subPredPair );
        if( it != cardinality.end() ) {
            it.value()++;
        }
        else
            cardinality.insert( subPredPair, 1 );
    }

    //
    // Check the cardinality + domain/range of remaining statements
    //
    ClassAndPropertyTree * tree = m_model->classAndPropertyTree();

    // FIXME: This is really inefficent
    foreach( const Soprano::Statement & st, remainingStatements ) {
        const QUrl subUri = getBlankOrResourceUri( st.subject() );
        const QUrl & propUri = st.predicate().uri();
        //
        // Check for Cardinality
        //
        QPair<QUrl,QUrl> subPredPair( subUri, propUri );

        int maxCardinality = tree->maxCardinality( propUri );

        if( maxCardinality > 0 ) {
            int existingCardinality = 0;
            if(!st.subject().isBlank()) {
                existingCardinality = m_model->executeQuery(QString::fromLatin1("select count(distinct ?v) where { %1 %2 ?v . FILTER(?v!=%3) . }")
                                                            .arg(st.subject().toN3(), st.predicate().toN3(), st.object().toN3()),
                                                            Soprano::Query::QueryLanguageSparql)
                    .iterateBindings(0)
                    .allNodes().first().literal().toInt();
            }
            const int stCardinality = cardinality.value( subPredPair );
            const int newCardinality = stCardinality + existingCardinality;

            if( newCardinality > maxCardinality) {
                // Special handling for max Cardinality == 1
                if( maxCardinality == 1 ) {
                    // If the difference is 1, then that is okay, as the OverwriteProperties flag
                    // has been set
                    if( (m_flags & OverwriteProperties) && (newCardinality-maxCardinality) == 1 ) {
                        continue;
                    }
                }

                // The LazyCardinalities flag has been set, we don't care about cardinalities any more
                if( (m_flags & LazyCardinalities) ) {
                    continue;
                }

                setError( QString::fromLatin1("%1 has a max cardinality of %2")
                                .arg( st.predicate().toString()).arg( maxCardinality ),
                                Soprano::Error::ErrorInvalidStatement);
                return false;
            }
        }

        //
        // Check for rdfs:domain and rdfs:range
        //

        QUrl domain = tree->propertyDomain( propUri );
        QUrl range = tree->propertyRange( propUri );

//        kDebug() << "Domain : " << domain;
//        kDebug() << "Range : " << range;

        QList<QUrl> subjectNewTypes = types.values( subUri );

        // domain
        if( !domain.isEmpty() && !isOfType( subUri, domain, subjectNewTypes ) ) {
            // Error
            QList<QUrl> allTypes = ( subjectNewTypes + existingTypes(subUri) );

            QString error = QString::fromLatin1("%1 has a rdfs:domain of %2. "
                                                "%3 only has the following types %4" )
                            .arg( Soprano::Node::resourceToN3( propUri ),
                                  Soprano::Node::resourceToN3( domain ),
                                  Soprano::Node::resourceToN3( subUri ),
                                  Nepomuk::resourcesToN3( allTypes ).join(", ") );
            setError( error, Soprano::Error::ErrorInvalidArgument);
            return false;
        }

        // range
        if( !range.isEmpty() ) {
            if( st.object().isResource() || st.object().isBlank() ) {
                const QUrl objUri = getBlankOrResourceUri( st.object() );
                QList<QUrl> objectNewTypes= types.values( objUri );

                if( !isOfType( objUri, range, objectNewTypes ) ) {
                    // Error
                    QList<QUrl> allTypes = ( objectNewTypes + existingTypes(objUri) );

                    QString error = QString::fromLatin1("%1 has a rdfs:range of %2. "
                                                "%3 only has the following types %4" )
                                    .arg( Soprano::Node::resourceToN3( propUri ),
                                          Soprano::Node::resourceToN3( range ),
                                          Soprano::Node::resourceToN3( objUri ),
                                          resourcesToN3( allTypes ).join(", ") );
                    setError( error, Soprano::Error::ErrorInvalidArgument );
                    return false;
                }
            }
            else if( st.object().isLiteral() ) {
                const Soprano::LiteralValue lv = st.object().literal();
                // Special handling for xsd:duration
                if( range == xsdDuration() && lv.isUnsignedInt() ) {
                    continue;
                }
                if( (!lv.isPlain() && lv.dataTypeUri() != range) ||
                        (lv.isPlain() && range != RDFS::Literal()) ) {
                    // Error
                    QString error = QString::fromLatin1("%1 has a rdfs:range of %2. "
                                                        "Provided %3")
                                    .arg( Soprano::Node::resourceToN3( propUri ),
                                          Soprano::Node::resourceToN3( range ),
                                          Soprano::Node::literalToN3(lv) );
                    setError( error, Soprano::Error::ErrorInvalidArgument);
                    return false;
                }
            }
        } // range

    } // foreach

    // The graph is error free.

    // FIXME: Do this later
    // Create all the blank nodes
    resolveBlankNodesInList( &remainingStatements );
    resolveBlankNodesInList( &typeStatements );
    resolveBlankNodesInList( &metadataStatements );


    //Merge its statements except for the resource metadata statements
    QList<Soprano::Statement> mergeStatements = remainingStatements + typeStatements;

    //
    // Graph Handling
    //
    QMultiHash<QUrl, Soprano::Statement> duplicateStatements;

    QMutableListIterator<Soprano::Statement> it( mergeStatements );
    while( it.hasNext() ) {
        const Soprano::Statement &st = it.next();

        const QString query = QString::fromLatin1("select ?g where { graph ?g { %1 %2 %3 . } . } LIMIT 1")
                              .arg(st.subject().toN3(),
                                   st.predicate().toN3(),
                                   st.object().toN3());

        Soprano::QueryResultIterator qit = m_model->executeQuery( query, Soprano::Query::QueryLanguageSparql);
        if(qit.next()) {
            const QUrl oldGraph = qit[0].uri();
            qit.close();

            duplicateStatements.insert( oldGraph, st );
            it.remove();
        }
    }

    //
    // Create all the graphs
    //
    QMutableHashIterator<QUrl, Soprano::Statement> hit( duplicateStatements );
    while( hit.hasNext() ) {
        hit.next();
        const QUrl& oldGraph = hit.key();

        if( mergeGraphs( oldGraph ) ) {
            const QUrl newGraph = m_graphHash[oldGraph];
            if( newGraph.isValid() ) {
                //kDebug() << "Is valid!  " << newGraph;
                //kDebug() << "Removing " << newSt;
                //model()->removeAllStatements( newSt.subject(), newSt.predicate(), newSt.object() );
            }
            else {
                // both the oldGraph, and newGraph are the same
                // Ignore this statement
                hit.remove();
            }
        }
        else
            return false;
    }

    // Create the main graph, if they are any statements to merge
    if( !mergeStatements.isEmpty() ) {
        m_graph = createGraph();
    }

    // Get a list of all the modified resources
    // ?

    // Push all these statements
    foreach( const Soprano::Statement & st, mergeStatements ) {
        push( st );
    }

    // Push all the duplicateStatements
    QHashIterator<QUrl, Soprano::Statement> hashIter( duplicateStatements );
    while( hashIter.hasNext() ) {
        hashIter.next();
        Soprano::Statement st = hashIter.value();

        m_model->removeAllStatements( st.subject(), st.predicate(), st.object(), hashIter.key() );
        const QUrl newGraph( m_graphHash[hashIter.key()] );
        st.setContext( newGraph );

        // Shouldn't we apply m_flag tests over here?
        m_model->addStatement( st );
    }

    //
    // Handle Resource metadata
    //

    // First update the mtime of all the modified resources
    Soprano::Node currentDateTime = Soprano::LiteralValue( QDateTime::currentDateTime() );
    foreach( const QUrl & resUri, m_modifiedResources ) {
        Soprano::Statement st( resUri, NAO::lastModified(), currentDateTime, graph() );
        addResMetadataStatement( st );
    }

    // then push the individual metadata statements
    foreach( Soprano::Statement st, metadataStatements ) {
        addResMetadataStatement( st );
    }

    return true;
}

Soprano::Error::ErrorCode Nepomuk::ResourceMerger::addStatement(const Soprano::Statement& st)
{
    m_modifiedResources << st.subject().uri();

    return model()->addStatement( st );
}


Soprano::Error::ErrorCode Nepomuk::ResourceMerger::addResMetadataStatement(const Soprano::Statement& st)
{
    const QUrl & predicate = st.predicate().uri();

    // Special handling for nao:lastModified and nao:userVisible: only the latest value is correct
    if( predicate == NAO::lastModified() ||
            predicate == NAO::userVisible() ) {
        model()->removeAllStatements( st.subject(), st.predicate(), Soprano::Node() );
    }

    // Special handling for nao:created: only the first value is correct
    else if( predicate == NAO::created() ) {
        // If nao:created already exists, then do nothing
        // FIXME: only write nao:created if we actually create the resource or if it was provided by the client, otherwise drop it.
        if( model()->containsAnyStatement( st.subject(), NAO::created(), Soprano::Node() ) )
            return Soprano::Error::ErrorNone;
    }

    // Special handling for nao:creator
    else if( predicate == NAO::creator() ) {
        // FIXME: handle nao:creator somehow
    }

    return model()->addStatement( st );
}
