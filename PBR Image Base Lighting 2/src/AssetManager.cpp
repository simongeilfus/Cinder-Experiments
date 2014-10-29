//
//  LiveDataSource.h
//  AssetAPI
//
//  Created by Simon Geilfus on 17/01/13.

#include "AssetManager.h"

using namespace std;
using namespace ci;

bool AssetManager::Loader::watch()
{
    time_t time = fs::last_write_time( app::getAssetPath( mRelativePath ) );
    if( time > mLastTimeWritten ){
        mLastTimeWritten   = time;
        return true;
    }
    return false;
}

void AssetManager::Loader::notify()
{
    mCallback( app::loadAsset( mRelativePath ) );
}

void AssetManager::load( const fs::path &relativePath, function<void(DataSourceRef)> callback, const Options &options )
{
    if( fs::exists( app::getAssetPath( relativePath ) ) ){
        LoaderRef loader;
        if( options.isAsynchronous() ){
            loader = LoaderRef( new AsyncLoader<void>( relativePath, callback, options ) );
            instance()->mAsyncLoaders.push_back( loader );
        }
        else
            loader = LoaderRef( new Loader( relativePath, callback, options ) );
        
        if( options.isWatching() )
            instance()->mWatchingLoaders.push_back( loader );
        
        loader->notify();
    }
    else throw app::AssetLoadExc( relativePath );
}


void AssetManager::load( const ci::fs::path &p0, const ci::fs::path &p1, std::function<void(ci::DataSourceRef,ci::DataSourceRef)> callback, const Options &options )
{
    load( { p0, p1 }, callback, options );    
}
void AssetManager::load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, std::function<void(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> callback, const Options &options )
{
    load( { p0, p1, p2 }, callback, options );
}
void AssetManager::load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, const ci::fs::path &p3, std::function<void(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> callback, const Options &options )
{
    load( { p0, p1, p2, p3 }, callback, options );
}


AssetManager* AssetManager::instance()
{
    if( !mInstance ){
        mInstance = new AssetManager();
        app::App::get()->getSignalUpdate().connect( bind( &AssetManager::update, mInstance ) );
    }
    return mInstance;
}

void AssetManager::update()
{
    for( deque<LoaderRef>::iterator it = mWatchingLoaders.begin(); it != mWatchingLoaders.end(); ++it ){
        bool needsReload = (*it)->watch();
        if( needsReload && (*it)->mOptions.isAsynchronous() && ( find( mAsyncLoaders.begin(), mAsyncLoaders.end(), *it ) == mAsyncLoaders.end() ) ){
            (*it)->notify();
            mAsyncLoaders.push_back( *it );
        }
        else if( needsReload ){
            (*it)->notify();
        }
    }
    for( deque<LoaderRef>::iterator it = mAsyncLoaders.begin(); it != mAsyncLoaders.end(); ){
        (*it)->update();
        if( (*it)->loaded() ){
            it = mAsyncLoaders.erase( it );
        }
        else ++it;
    }
}

AssetManager* AssetManager::mInstance;
