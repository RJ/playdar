/***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#define private public
    #include "ITunesPlaysDatabase.h"
#undef private

#include "Moose.h"

#include <QtTest>
#include <QtSql>


class TestITunesPlaysDatabaseMac : public QObject
{
    Q_OBJECT

    public:
        TestITunesPlaysDatabaseMac();

    private slots:
        void testIsValid();

    private:
        ITunesPlaysDatabase m_db;

        inline void createTable()
        {
            QSqlQuery query; 
            query.exec( "CREATE TABLE itunes_db ( "
                        "persistent_id   VARCHAR( 32 ) PRIMARY KEY,"
                        "path            TEXT,"
                        "play_count      INTEGER )" );
        }
                                                    
        inline void deleteTable()
        {
            QSqlQuery query;
            query.exec( "DROP TABLE itunes_db" );
        }
};


TestITunesPlaysDatabaseMac::TestITunesPlaysDatabaseMac()
{
    QSqlDatabase qdb = QSqlDatabase::addDatabase( "QSQLITE" );
    qdb.setDatabaseName ( QString( Moose::applicationSupport().c_str() ) + "iTunesPlays.db" );
    bool ok = qdb.open();
}


void TestITunesPlaysDatabaseMac::testIsValid()
{
    
    deleteTable();
    
    QCOMPARE( m_db.isValid(), false );
    
    createTable();
                    
    QCOMPARE( m_db.isValid(), true );
}


QTEST_MAIN( TestITunesPlaysDatabaseMac )

#include "TestITunesPlaysDatabaseMac.moc"
