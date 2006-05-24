#include "../spellbook.h"

#include <QDebug>

class spSanitizeLinkArrays20000005 : public Spell
{
public:
	QString name() const { return "Adjust Link Arrays"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->checkVersion( 0x14000005, 0x14000005 ) && ! index.isValid();
	}
	
	static bool compareChildLinks( QPair<qint32, bool> a, QPair<qint32, bool> b )
	{
		return a.second != b.second ? a.second : a.first < b.first;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ )
		{
			QModelIndex iBlock = nif->getBlock( n );
			
			// reorder children (tribasedgeom first)
			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );
			if ( iNumChildren.isValid() && iChildren.isValid() )
			{
				QList< QPair<qint32, bool> > links;
				for ( int r = 0; r < nif->rowCount( iChildren ); r++ )
				{
					qint32 l = nif->getLink( iChildren.child( r, 0 ) );
					if ( l >= 0 )
						links.append( QPair<qint32, bool>( l, nif->inherits( nif->getBlock( l ), "NiTriBasedGeom" ) ) );
				}
				
				qStableSort( links.begin(), links.end(), compareChildLinks );
				
				for ( int r = 0; r < links.count(); r++ )
				{
					if ( links[r].first != nif->getLink( iChildren.child( r, 0 ) ) )
						nif->setLink( iChildren.child( r, 0 ), links[r].first );
					nif->set<int>( iNumChildren, links.count() );
					nif->updateArray( iChildren );
				}
			}
			
			// remove empty property links
			QModelIndex iNumProperties = nif->getIndex( iBlock, "Num Properties" );
			QModelIndex iProperties = nif->getIndex( iBlock, "Properties" );
			if ( iNumProperties.isValid() && iProperties.isValid() )
			{
				QList<qint32> links;
				for ( int r = 0; r < nif->rowCount( iProperties ); r++ )
				{
					qint32 l = nif->getLink( iProperties.child( r, 0 ) );
					if ( l >= 0 ) links.append( l );
				}
				if ( links.count() < nif->rowCount( iProperties ) )
				{
					for ( int r = 0; r < links.count(); r++ )
						nif->setLink( iProperties.child( r, 0 ), links[r] );
					nif->set<int>( iNumProperties, links.count() );
					nif->updateArray( iProperties );
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spSanitizeLinkArrays20000005 )

class spSanityCheckLinks : public Spell
{
public:
	QString name() const { return "Check Links"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		for ( int b = 0; b < nif->getBlockCount(); b++ )
		{
			QModelIndex iBlock = nif->getBlock( b );
			QModelIndex idx = check( nif, iBlock );
			if ( idx.isValid() )
				return idx;
		}
		return QModelIndex();
	}
	
	QModelIndex check( NifModel * nif, const QModelIndex & iParent )
	{
		for ( int r = 0; r < nif->rowCount( iParent ); r++ )
		{
			QModelIndex idx = iParent.child( r, 0 );
			bool child;
			if ( nif->isLink( idx, &child ) )
			{
				qint32 l = nif->getLink( idx );
				if ( l < 0 )
				{
					if ( ! child )
					{
						qWarning() << "unassigned parent link";
						return idx;
					}
				}
				else if ( l >= nif->getBlockCount() )
				{
					qWarning() << "invalid link";
					return idx;
				}
				else
				{
					QString tmplt = idx.sibling( idx.row(), NifModel::TempCol ).data( Qt::DisplayRole ).toString();
					if ( ! tmplt.isEmpty() )
					{
						QModelIndex iBlock = nif->getBlock( l );
						if ( ! nif->inherits( iBlock, tmplt ) )
						{
							qWarning() << "link points to wrong block type";
							return idx;
						}
					}
				}
			}
			if ( nif->rowCount( idx ) > 0 )
			{
				QModelIndex x = check( nif, idx );
				if ( x.isValid() )
					return x;
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spSanityCheckLinks )
