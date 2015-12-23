/*
 
 Watchdog
 
 Copyright (c) 2014, Simon Geilfus
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <map>
#include <string>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>

#ifdef CINDER_CINDER
    #include "cinder/Filesystem.h"
    #if CINDER_VERSION < 900
        #include "cinder/app/AppNative.h"
    #else
        #include "cinder/app/App.h"
    #endif
#else
    #if defined( CINDER_WINRT )
        #include <filesystem>
        namespace ci { namespace fs = std::tr2::sys; }
    #else
        #define BOOST_FILESYSTEM_VERSION 3
        #define BOOST_FILESYSTEM_NO_DEPRECATED
        #include <boost/filesystem.hpp>
        namespace ci { namespace fs = boost::filesystem; }
    #endif
#endif

// Windows Issue :
// For the moment the overloaded version of wd::watch has a different name on windows
// platforms because of some issues with visual studio lambda support (see above)
// see https://forum.libcinder.org/topic/watchdog#23286000002228083
// and https://github.com/simongeilfus/Watchdog/issues/2
// It seems that visual studio is not able to resolve the ambiguity between the two
// wd::watch methods when trying to cast a lambda to a std::function. A quick
// temporary way to fix the issue is to rename the method on windows until microsoft
// decide to update visual studio compiler.
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define WIN_AMBIGUITY_FIX

    // Also disable an annoying warning:
    #pragma warning( push )
    #pragma warning( disable:4996 ) // warning C4996: 'sprintf': This function or variable may be unsafe.
#endif

// There's currently some issues with this so it's disabled for now :
// By default watchdog is disabled in release mode and will only execute the
// provided callback once when wd::watch is called and do nothing for the
// other methods. Undef this if you want Watchdog to work in release mode.
// #define WATCHDOG_ONLY_IN_DEBUG

//! Exception for when Watchdog can't locate a file or parse the wildcard
class WatchedFileSystemExc : public std::exception {
public:
    WatchedFileSystemExc( const ci::fs::path &path )
    {
        sprintf( mMessage, "Failed to find file or directory at: %s", path.c_str() );
    }
    
    virtual const char * what() const throw() { return mMessage; }
    
    char mMessage[4096];
};

//! Watchdog class. To be able to benefit from the WATCHDOG_ONLY_IN_DEBUG mechanism you should use wd instead of Watchdog
class Watchdog {
public:
    
    //! Watches a file or directory for modification and call back the specified std::function. The path specified is passed as argument of the callback even if there is multiple files. Use the second watch method if you want to receive a list of all the files that have been modified.
    static void watch( const ci::fs::path &path, const std::function<void(const ci::fs::path&)> &callback )
    {
        watchImpl( path, callback, std::function<void(const std::vector<ci::fs::path>&)>() );
    }
    
    //! Watches a file or directory for modification and call back the specified std::function. A list of modified files or directory is passed as argument of the callback. Use this version only if you are watching multiple files or a directory.
#ifdef WIN_AMBIGUITY_FIX
    static void watchMany( const ci::fs::path &path, const std::function<void(const std::vector<ci::fs::path>&)> &callback )
#else
    static void watch( const ci::fs::path &path, const std::function<void(const std::vector<ci::fs::path>&)> &callback )
#endif
    {
        watchImpl( path, std::function<void(const ci::fs::path&)>(), callback );
    }
    //! Unwatches a previously registrated file or directory
    static void unwatch( const ci::fs::path &path )
    {
        watchImpl( path );
    }
    //! Unwatches all previously registrated file or directory
    static void unwatchAll()
    {
        watchImpl( ci::fs::path() );
    }
    //! Sets the last modification time of a file or directory. by default sets the time to the current time
    static void touch( const ci::fs::path &path, std::time_t time = std::time( nullptr ) )
    {
        
        // if the file or directory exists change its last write time
        if( ci::fs::exists( path ) ){
            ci::fs::last_write_time( path, time );
            return;
        }
        // if not, visit each path if there's a wildcard
        if( path.string().find( "*" ) != std::string::npos ){
            visitWildCardPath( path, [time]( const ci::fs::path &p ){
                ci::fs::last_write_time( p, time );
                return false;
            } );
        }
        // otherwise throw an exception
        else {
            throw WatchedFileSystemExc( path );
        }
    }
    
protected:
    
    Watchdog()
    : mWatching(false)
    {
    }
    
    void close()
    {
        // remove all watchers
        unwatchAll();
        
        // stop the thread
        mWatching = false;
        if( mThread->joinable() ) mThread->join();
    }
    
    
    void start()
    {
        mWatching   = true;
        mThread     = std::unique_ptr<std::thread>( new std::thread( [this](){
            // keep watching for modifications every ms milliseconds
            auto ms = std::chrono::milliseconds( 500 );
            while( mWatching ) {
                do {
                    // iterate through each watcher and check for modification
                    std::lock_guard<std::mutex> lock( mMutex );
                    auto end = mFileWatchers.end();
                    for( auto it = mFileWatchers.begin(); it != end; ++it ) {
                        it->second.watch();
                    }
                    // lock will be released before this thread goes to sleep
                } while( false );
                
                // make this thread sleep for a while
                std::this_thread::sleep_for( ms );
            }
        } ) );
    }
    static void watchImpl( const ci::fs::path &path, const std::function<void(const ci::fs::path&)> &callback = std::function<void(const ci::fs::path&)>(), const std::function<void(const std::vector<ci::fs::path>&)> &listCallback = std::function<void(const std::vector<ci::fs::path>&)>() )
    {
        // create the static Watchdog instance
        static Watchdog wd;
        // and start its thread
        if( !wd.mWatching ) {
            wd.start();
            #ifdef CINDER_CINDER
                #if CINDER_VERSION < 900
                    ci::app::App::get()->getSignalShutdown().connect( [&]() {
                #else
                    ci::app::App::get()->getSignalCleanup().connect( [&]() {
                #endif
                        wd.close();
                    } );
            #endif
        }
        
        const std::string key = path.string();
        
        // add a new watcher
        if( callback || listCallback ){
            
            std::string filter;
            ci::fs::path p = path;
            // try to see if there's a match for the wildcard
            if( path.string().find( "*" ) != std::string::npos ){
                bool found = false;
                std::pair<ci::fs::path,std::string> pathFilter = visitWildCardPath( path, [&found]( const ci::fs::path &p ){
                    found = true;
                    return true;
                } );
                if( !found ){
                    throw WatchedFileSystemExc( path );
                }
                else {
                    p       = pathFilter.first;
                    filter  = pathFilter.second;
                }
            }
            
#ifdef CINDER_CINDER
            // try to see if the path is an asset
            if( !ci::fs::exists( p ) ){
                ci::fs::path asset = ci::app::getAssetPath( p );
                if( !asset.empty() ){
                    p = asset;
                }
            }
            // throw an exception if the file doesn't exist
            if( !ci::fs::exists( p ) ){
                throw WatchedFileSystemExc( path );
            }
#endif
            
            std::lock_guard<std::mutex> lock( wd.mMutex );
            if( wd.mFileWatchers.find( key ) == wd.mFileWatchers.end() ){
                wd.mFileWatchers.emplace( make_pair( key, Watcher( p, filter, callback, listCallback ) ) );
            }
        }
        // if there is no callback that means that we are unwatching
        else {
            // if the path is empty we unwatch all files
            if( path.empty() ){
                std::lock_guard<std::mutex> lock( wd.mMutex );
                for( auto it = wd.mFileWatchers.begin(); it != wd.mFileWatchers.end(); ) {
                    it = wd.mFileWatchers.erase( it );
                }
            }
            // or the specified file or directory
            else {
                std::lock_guard<std::mutex> lock( wd.mMutex );
                auto watcher = wd.mFileWatchers.find( key );
                if( watcher != wd.mFileWatchers.end() ){
                    wd.mFileWatchers.erase( watcher );
                }
            }
        }
    }
    
    static std::pair<ci::fs::path,std::string> getPathFilterPair( const ci::fs::path &path )
    {
        // extract wildcard and parent path
        std::string key     = path.string();
        ci::fs::path p      = path;
        size_t wildCardPos  = key.find( "*" );
        std::string filter;
        if( wildCardPos != std::string::npos ){
            filter  = path.filename().string();
            p       = path.parent_path();
        }
        
#ifdef CINDER_CINDER
        // try to see if the path is an asset
        if( !ci::fs::exists( p ) ){
            ci::fs::path asset = ci::app::getAssetPath( p );
            if( !asset.empty() ){
                p = asset;
            }
        }
#endif
        // throw an exception if the file doesn't exist
        if( filter.empty() && !ci::fs::exists( p ) ){
            throw WatchedFileSystemExc( path );
        }
#ifdef CINDER_CINDER
        else if( !filter.empty() && p.empty() ) {
            // at this stage if the parent directory doesn't exist the only option left is to try with the asset folder
            p = ci::app::getAssetPath( "" );
        }
#endif
        
        return std::make_pair( p, filter );
            
    }
    
    static std::pair<ci::fs::path,std::string> visitWildCardPath( const ci::fs::path &path, const std::function<bool(const ci::fs::path&)> &visitor ){
        std::pair<ci::fs::path, std::string> pathFilter = getPathFilterPair( path );
        if( !pathFilter.second.empty() ){
            std::string full    = ( pathFilter.first / pathFilter.second ).string();
            size_t wildcardPos  = full.find( "*" );
            std::string before  = full.substr( 0, wildcardPos );
            std::string after   = full.substr( wildcardPos + 1 );
            ci::fs::directory_iterator end;
            for( ci::fs::directory_iterator it( pathFilter.first ); it != end; ++it ){
                std::string current = it->path().string();
                size_t beforePos    = current.find( before );
                size_t afterPos     = current.find( after );
                if( ( beforePos != std::string::npos || before.empty() )
                   && ( afterPos != std::string::npos || after.empty() ) ) {
                    if( visitor( it->path() ) ){
                        break;
                    }
                }
            }
        }
        return pathFilter;
    }
    
    class Watcher {
    public:
        Watcher( const ci::fs::path &path, const std::string &filter, const std::function<void(const ci::fs::path&)> &callback, const std::function<void(const std::vector<ci::fs::path>&)> &listCallback )
        : mPath(path), mFilter(filter), mCallback(callback), mListCallback(listCallback)
        {
            // make sure we store all initial write time
            if( !mFilter.empty() ) {
                std::vector<ci::fs::path> paths;
                visitWildCardPath( path / filter, [this,&paths]( const ci::fs::path &p ){
                    hasChanged( p );
                    paths.push_back( p );
                    return false;
                } );
                // this means that the first watch won't call the callback function
                // so we have to manually call it here
                if( mCallback ){
                    mCallback( mPath / mFilter );
                }
                else {
                    mListCallback( paths );
                }
            }
        }
        
        void watch()
        {
            // if there's no filter we just check for one item
            if( mFilter.empty() && hasChanged( mPath ) && mCallback ){
#ifdef CINDER_CINDER
                ci::app::App::get()->dispatchAsync( [this](){
                    mCallback( mPath );
                } );
#else
                mCallback( mPath );
                //#error TODO: still have to figure out an elegant way to do this without cinder
#endif
            }
            // otherwise we check the whole parent directory
            else if( !mFilter.empty() ){
                
                std::vector<ci::fs::path> paths;
                visitWildCardPath( mPath / mFilter, [this,&paths]( const ci::fs::path &p ){
                    bool pathHasChanged = hasChanged( p );
                    if( pathHasChanged && mCallback ){
#ifdef CINDER_CINDER
                        ci::app::App::get()->dispatchAsync( [this](){
                            mCallback( mPath / mFilter );
                        } );
#else
                        mCallback( mPath / mFilter );
                        //#error TODO: still have to figure out an elegant way to do this without cinder
#endif
                        return true;
                    }
                    else if( pathHasChanged && mListCallback ){
                        paths.push_back( p );
                    }
                    return false;
                } );
                if( paths.size() && mListCallback ){
                    mListCallback( paths );
                }
            }
            
        }
        
        bool hasChanged( const ci::fs::path &path )
        {
            // get the last modification time
            std::time_t time = ci::fs::last_write_time( path );
            // add a new modification time to the map
            std::string key = path.string();
            if( mModificationTimes.find( key ) == mModificationTimes.end() ) {
                mModificationTimes[ key ] = time;
                return true;
            }
            // or compare with an older one
            auto &prev = mModificationTimes[ key ];
            if( prev < time ) {
                prev = time;
                return true;
            }
            return false;
        };
        
    protected:
        ci::fs::path                                            mPath;
        std::string                                             mFilter;
        std::function<void(const ci::fs::path&)>                mCallback;
        std::function<void(const std::vector<ci::fs::path>&)>   mListCallback;
        std::map< std::string, time_t >                         mModificationTimes;
    };
    
    friend class SleepyWatchdog;
    
    std::mutex                      mMutex;
    std::atomic<bool>               mWatching;
    std::unique_ptr<std::thread>    mThread;
    std::map<std::string,Watcher>   mFileWatchers;
};

//! this class is only used in release mode when WATCHDOG_ONLY_IN_DEBUG is defined
class SleepyWatchdog {
public:
    
    //! executes the callback once
    static void watch( const ci::fs::path &path, const std::function<void(const ci::fs::path&)> &callback )
    {
        auto pathFilter = Watchdog::visitWildCardPath( path, []( const ci::fs::path &p ){ return false; } );
        if( pathFilter.first.empty() ){
            throw WatchedFileSystemExc( path );
        }
        else {
            callback( pathFilter.first );
        }
    }
#ifdef WIN_AMBIGUITY_FIX
    static void watchMany( const ci::fs::path &path, const std::function<void(const std::vector<ci::fs::path>&)> &callback )
#else
    static void watch( const ci::fs::path &path, const std::function<void(const std::vector<ci::fs::path>&)> &callback )
#endif
    {
        auto pathFilter = Watchdog::visitWildCardPath( path, []( const ci::fs::path &p ){ return false; } );
        if( pathFilter.first.empty() ){
            throw WatchedFileSystemExc( path );
        }
        else {
            // TODO: this is wrong
            callback( std::vector<ci::fs::path>() );
        }
    }
    //! does nothing
    static void unwatch( const ci::fs::path &path ) {}
    
    //! does nothing
    static void unwatchAll() {}
    
    //! does nothing
    static void touch( const ci::fs::path &path, std::time_t time = std::time( nullptr ) ) {}
};

// defines the macro that allow to change the RELEASE/DEBUG behavior
#ifdef WATCHDOG_ONLY_IN_DEBUG
    #if defined(NDEBUG) || defined(_NDEBUG) || defined(RELEASE) || defined(MASTER) || defined(GOLD)
        #define wd SleepyWatchdog
    #else
        #define wd Watchdog
    #endif
#else
    typedef Watchdog wd;
#endif
	
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #pragma warning( pop )
#endif