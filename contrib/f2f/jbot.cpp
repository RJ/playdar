#include "jbot.h"
#include <boost/foreach.hpp>

using namespace gloox;
using namespace std;

jbot::jbot(std::string jid, std::string pass)
        : m_jid(jid), m_pass(pass) {}

void
jbot::start()
{
    JID jid( m_jid );
    j = new Client( jid, m_pass );
    j->registerConnectionListener( this );
    j->registerMessageHandler( this );
    j->rosterManager()->registerRosterListener( this );
    j->disco()->registerDiscoHandler( this );

    j->disco()->setVersion( "gloox_playdar", GLOOX_VERSION, "Linux" );
    j->disco()->setIdentity( "client", "bot" );
    j->disco()->addFeature( "playdar:resolver" );

    j->logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this );

    if ( j->connect( false ) )
    {
        ConnectionError ce = ConnNoError;
        while ( ce == ConnNoError )
        {
            ce = j->recv();
        }
        printf( "ce: %d\n", ce );
    }

    delete( j );
}

void 
jbot::stop()
{
    j->disconnect();
}

/// send msg to specific jid
void
jbot::send_to( const string& to, const string& msg )
{
    Message m(Message::Chat, JID(to), msg, "");
    j->send( m ); //FIXME no idea if this is threadsafe. need to RTFM
}

/// send msg to all playdarpeers
void 
jbot::broadcast_msg( const std::string& msg )
{
    const string subject = "";
    BOOST_FOREACH( PlaydarPeer& p, m_playdarpeers )
    {
        printf("Dispatching query to: %s\n", p.jid.full().c_str() );
        Message m(Message::Chat, p.jid, msg, subject);
        j->send( m );
    }
}

void 
jbot::set_msg_received_callback( boost::function<void(const std::string&, const std::string&)> cb)
{
    m_msg_received_callback = cb;
}

void 
jbot::clear_msg_received_callback()
{
    m_msg_received_callback = 0;
}

/// GLOOXY CALLBACKS FOLLOW

void 
jbot::onConnect()
{
    printf( "connected!!!\n" );
    // broadcast our presence (extended away) with the lowest priority
    //j->setPresence( Presence::XA, -128, "I'm a bot, not a human." );
}

void 
jbot::onDisconnect( ConnectionError e )
{
    printf( "jbot: disconnected: %d\n", e );
    if ( e == ConnAuthenticationFailed )
        printf( "auth failed. reason: %d\n", j->authError() );
}

bool 
jbot::onTLSConnect( const CertInfo& info )
{
    printf( "status: %d\nissuer: %s\npeer: %s\nprotocol: %s\nmac: %s\ncipher: %s\ncompression: %s\n"
            "from: %s\nto: %s\n",
            info.status, info.issuer.c_str(), info.server.c_str(),
            info.protocol.c_str(), info.mac.c_str(), info.cipher.c_str(),
            info.compression.c_str(), ctime( (const time_t*)&info.date_from ),
            ctime( (const time_t*)&info.date_to ) );
    onConnect();
    return true;
}

void 
jbot::handleMessage( const Message& msg, MessageSession * /*session*/ )
{
    printf( "from: %s, type: %d, subject: %s, message: %s, thread id: %s\n",
            msg.from().full().c_str(), msg.subtype(),
            msg.subject().c_str(), msg.body().c_str(), msg.thread().c_str() );

    if( msg.from().bare() == JID(m_jid).bare() )
    {
        printf("Message is from ourselves/considered safe\n");
        //TODO admin interface using text commands? stats?
        if ( msg.body() == "quit" ) { stop(); return; }
    }
    
    if( m_msg_received_callback )
    {
        m_msg_received_callback( msg.body(), msg.from().full() );
    }

/*
    std::string re = "You said:\n> " + msg.body() + "\nI like that statement.";
    std::string sub;
    if ( !msg.subject().empty() )
        sub = "Re: " +  msg.subject();

    m_messageEventFilter->raiseMessageEvent( MessageEventDisplayed );
#if defined( WIN32 ) || defined( _WIN32 )
    Sleep( 1000 );
#else
    sleep( 1 );
#endif
    m_messageEventFilter->raiseMessageEvent( MessageEventComposing );
    m_chatStateFilter->setChatState( ChatStateComposing );
#if defined( WIN32 ) || defined( _WIN32 )
    Sleep( 2000 );
#else
    sleep( 2 );
#endif
    m_session->send( re, sub );

    if ( msg.body() == "quit" )
        j->disconnect();
*/
}

void 
jbot::handleLog( LogLevel level, LogArea area, const std::string& message )
{
    printf("log: level: %d, area: %d, %s\n", level, area, message.c_str() );
}

/// ROSTER STUFF
// {{{
void 
jbot::onResourceBindError( ResourceBindError error )
{
    printf( "onResourceBindError: %d\n", error );
}

void 
jbot::onSessionCreateError( SessionCreateError error )
{
    printf( "onSessionCreateError: %d\n", error );
}

void 
jbot::handleItemSubscribed( const JID& jid )
{
    printf( "subscribed %s\n", jid.bare().c_str() );
}

void 
jbot::handleItemAdded( const JID& jid )
{
    printf( "added %s\n", jid.bare().c_str() );
}

void 
jbot::handleItemUnsubscribed( const JID& jid )
{
    printf( "unsubscribed %s\n", jid.bare().c_str() );
}

void 
jbot::handleItemRemoved( const JID& jid )
{
    printf( "removed %s\n", jid.bare().c_str() );
}

void 
jbot::handleItemUpdated( const JID& jid )
{
    printf( "updated %s\n", jid.bare().c_str() );
}
// }}}
void 
jbot::handleRoster( const Roster& roster )
{
    printf( "roster arriving\nitems:\n" );
    Roster::const_iterator it = roster.begin();
    for ( ; it != roster.end(); ++it )
    {
        if ( (*it).second->subscription() != S10nBoth ) continue;
        j->disco()->getDiscoInfo( (*it).second->jid(), "", this, 0 );
        printf( "jid: %s, name: %s, subscription: %d\n",
                (*it).second->jid().c_str(), (*it).second->name().c_str(),
                (*it).second->subscription() );
        StringList g = (*it).second->groups();
        StringList::const_iterator it_g = g.begin();
        for ( ; it_g != g.end(); ++it_g )
            printf( "\tgroup: %s\n", (*it_g).c_str() );
        RosterItem::ResourceMap::const_iterator rit = (*it).second->resources().begin();
        for ( ; rit != (*it).second->resources().end(); ++rit )
            printf( "resource: %s\n", (*rit).first.c_str() );
    }
}

void 
jbot::handleRosterError( const IQ& /*iq*/ )
{
    printf( "a roster-related error occured\n" );
}

void 
jbot::handleRosterPresence( const RosterItem& item, const std::string& resource,
        Presence::PresenceType presence, const std::string& /*msg*/ )
{
    // does this presence mean they are offline/un-queryable:
    if ( presence == Presence::Unavailable ||
            presence == Presence::Error ||
            presence == Presence::Invalid )
    {
        printf( "//////presence received (->OFFLINE): %s/%s\n", item.jid().c_str(), resource.c_str() );
        // remove peer from list, if he exists
        for(std::vector<PlaydarPeer>::iterator  it = m_playdarpeers.begin();
            it != m_playdarpeers.end(); ++it )
        {
            if( it->jid == item.jid() )
            {
                printf("Removing from playdarpeers\n");
                m_playdarpeers.erase( it );
                break;
            }
        }
        return;
    }
    // If they just became available, disco them, otherwise ignore - they may
    // have just changed their status to Away or something asinine.
    BOOST_FOREACH( PlaydarPeer & p, m_playdarpeers )
    {
        if( p.jid == item.jid() ) return; // already online and in our list.
    }
    printf( "//////presence received (->ONLINE) %s/%s -- %d\n",
     item.jid().c_str(),resource.c_str(), presence );
    // Disco them to check if they are playdar-capable
    JID jid( item.jid() );
    jid.setResource( resource );
    j->disco()->getDiscoInfo( jid, "", this, 0 );
}

void 
jbot::handleSelfPresence( const RosterItem& item, const std::string& resource,
                                       Presence::PresenceType presence, const std::string& msg )
{
    printf( "self presence received: %s/%s -- %d\n", item.jid().c_str(), resource.c_str(), presence );
    handleRosterPresence( item, resource, presence, msg );
}

bool 
jbot::handleSubscriptionRequest( const JID& jid, const std::string& /*msg*/ )
{
    printf( "subscription: %s\n", jid.bare().c_str() );
    StringList groups;
    JID id( jid );
    j->rosterManager()->subscribe( id, "", groups, "" );
    return true;
}

bool 
jbot::handleUnsubscriptionRequest( const JID& jid, const std::string& /*msg*/ )
{
    printf( "unsubscription: %s\n", jid.bare().c_str() );
    return true;
}

void 
jbot::handleNonrosterPresence( const Presence& presence )
{
    printf( "received presence from entity not in the roster: %s\n", presence.from().full().c_str() );
}
/// END ROSTER STUFF


/// DISCO STUFF
void 
jbot::handleDiscoInfo( const JID& from, const Disco::Info& info, int context)
{
    printf("///////// handleDiscoInfo!\n");
    if ( info.hasFeature("playdar:resolver") || true ) // FIXME testing hack
    {
        printf( "Found contact with playdar capabilities! '%s'\n", from.full().c_str() );
        bool found = false;
        BOOST_FOREACH( PlaydarPeer p, m_playdarpeers )
        {
            if( p.jid.full() == from.full() )
            {
                found = true;
                break;
            }
        }
        if( !found )
        {
            PlaydarPeer p;
            p.jid = from;
            m_playdarpeers.push_back( p );
        }
    }
    
    printf("Available playdar JIDs: ");
    BOOST_FOREACH( PlaydarPeer p, m_playdarpeers )
    {
        printf("%s ", p.jid.full().c_str() );
    }
    printf("\n");
    
}

void 
jbot::handleDiscoItems( const JID& /*iq*/, const Disco::Items&, int /*context*/ )
{
    printf( "handleDiscoItemsResult\n" );
}

void 
jbot::handleDiscoError( const JID& /*iq*/, const Error*, int /*context*/ )
{
    printf( "handleDiscoError\n" );
}
/// END DISCO STUFF


int main( int argc, char** argv )
{
  if(argc!=3)
  {
    printf("Usage: %s <jid> <pass>\n", *argv);
    return 1;
  }
  jbot *r = new jbot(argv[1], argv[2]);
  r->start();
  delete( r );
  return 0;
}
