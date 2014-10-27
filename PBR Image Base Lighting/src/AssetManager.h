/*
 
 AssetManager API
 
 Copyright (c) 2013, Simon Geilfus
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
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


#include "cinder/Filesystem.h"
#include "cinder/DataSource.h"
#include "cinder/app/App.h"


// TODO Support Url. AssetManager::load( Url( ... ) );

class AssetManager {
public:
    
    //! Options passed when creating an asset Loader.
    struct Options {
        Options() : mWatch( true ), mAsynchronous( false ) {}
        
        //! Sets whether the asset should be watched for hot reloading.
        Options& watch( bool watch = true )         { mWatch = watch; return *this; }
        //! Sets whether the asset should be asynchronously loaded.
        Options& asynchronous( bool async = true )  { mAsynchronous = async; return *this; }
        
        //! Returns whether the asset will be watched for hot reloading.
        bool    isWatching()        const { return mWatch; }
        //! Returns whether the asset will be asynchronously loaded.
        bool    isAsynchronous()    const { return mAsynchronous; }
        
    private:
        bool mAsynchronous;
        bool mWatch;
    };
    
    //! Loads an asset and if needed watch for hot reload.
    static void     load( const ci::fs::path &relativePath, std::function<void(ci::DataSourceRef)> callback, const Options &options = Options() );
    
    //! Load several assets and if needed watch for hot reload.
    static void                                 load( const ci::fs::path &p0, const ci::fs::path &p1, std::function<void(ci::DataSourceRef,ci::DataSourceRef)> callback, const Options &options = Options() );
    static void                                 load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, std::function<void(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> callback, const Options &options = Options() );
    static void                                 load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, const ci::fs::path &p3, std::function<void(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> callback, const Options &options = Options() );
    template<typename... Arguments> static void load( const std::vector<ci::fs::path> &relativePaths, std::function<void(Arguments...)> callback, const Options &options = Options() );
    
    
    //! Asynchronously loads an asset and if needed watch for hot reload.
    template<typename T> static void    load( const ci::fs::path &relativePath, std::function<T(ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options = Options()  );
    
    //! Asynchronously load several asset and if needed watch for hot reload.
    template<typename T> static void                        load( const ci::fs::path &p0, const ci::fs::path &p1, std::function<T(ci::DataSourceRef,ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options = Options()  );
    template<typename T> static void                        load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, std::function<T(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options = Options()  );
    template<typename T> static void                        load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, const ci::fs::path &p3, std::function<T(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options = Options()  );
    template<typename T, typename... Arguments> static void load( const std::vector<ci::fs::path> &relativePaths, std::function<T(Arguments...)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options = Options()  );
    
protected:
    
    //! Loader
    class Loader {
    public:
        Loader(){}
        Loader( ci::fs::path relativePath, std::function<void(ci::DataSourceRef)> callback, const Options &options = Options() )
        : mRelativePath( relativePath ), mCallback( callback ), mLastTimeWritten( ci::fs::last_write_time( ci::app::getAssetPath( relativePath ) ) ), mOptions( options ), mLoaded( false ) {}
        
        //! Returns whether the asset needs to be reloaded or not.
        virtual bool watch();
        //! Loads the asset and Notify the Callback function with the new DataSourceRef.
        virtual void notify();
        //! Checks for completeness and other features presents in subclasses.
        virtual void update(){}
        //! Returns whether the asset has been or not.
        virtual bool loaded() const { return mLoaded; }
        
    protected:
        friend class AssetManager;
        
        bool                                    mLoaded;
        Options                                 mOptions;
        
        ci::fs::path                            mRelativePath;
        std::time_t                             mLastTimeWritten;
        std::function<void(ci::DataSourceRef)>  mCallback;
    };
    
    //! AsyncLoader
    template<typename T>
    class AsyncLoader : public Loader {
    public:
        AsyncLoader( ci::fs::path relativePath, std::function<T(ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options = Options() )
        : mCallback( asyncCallback ), mMainThreadCallback( mainThreadCallback )
        {
            mRelativePath       = relativePath;
            mLastTimeWritten    = ci::fs::last_write_time( ci::app::getAssetPath( relativePath ) );
            mOptions            = options;
            mOptions.asynchronous();
        }
        
        //! Loads the asset in a separated thread and Notify the Callback function with the new DataSourceRef.
        virtual void notify()
        {
            mLoaded = false;
            std::thread( [this]() {
                ci::ThreadSetup threadSetup;
                ci::DataSourceRef data  = ci::app::loadAsset( mRelativePath );
                mAsyncData              = mCallback( data );
                mLoaded                 = true;
            } ).detach();
        }
        
        virtual void update()
        {
            if( loaded() ){
                if( mMainThreadCallback )
                    mMainThreadCallback( mAsyncData );
            }
        }
        
    protected:
        T                                    mAsyncData;
        std::function<void(T)>               mMainThreadCallback;
        std::function<T(ci::DataSourceRef)>  mCallback;
    };
    
    //! VariadicLoader Helper. Allows to dynamically call a variadic function with N parameters.
    template <uint N, typename T>
    struct VariadicHelper
    {
        //! VariadicLoader Helper start point (<N-1,...>)
        template <typename... Arguments, typename... Args >
        static T load( const std::vector<ci::fs::path> &relativePaths, const std::function<T(Arguments...)> &callback, Args... args )
        {
            return VariadicHelper<N-1,T>::load( relativePaths, callback, ci::app::loadAsset( relativePaths[N-1] ), args... );
        }
    };
    
    //! VariadicLoader
    template<typename... Arguments>
    class VariadicLoader : public Loader {
    public:
        VariadicLoader( const std::vector<ci::fs::path> &relativePaths, std::function<void(Arguments...)> callback, const Options &options = Options() )
        : mCallback( callback ), mRelativePaths( relativePaths )
        {
            for( auto path : relativePaths ){
                mLastTimesWritten.push_back( ci::fs::last_write_time( ci::app::getAssetPath( path ) ) );
            }
            mOptions = options;
        }
        
        
        //! Returns whether the asset needs to be reloaded or not.
        virtual bool watch(){
            bool needsReload = false;
            for( int i = 0; i < mLastTimesWritten.size(); i++ ){
                std::time_t time = ci::fs::last_write_time( ci::app::getAssetPath( mRelativePaths[i] ) );
                if( time > mLastTimesWritten[i] ){
                    mLastTimesWritten[i] = time;
                    needsReload = true;
                }
            }
            return needsReload;
        }
        
        //! Loads the asset and Notify the Callback function with the new DataSourceRef.
        virtual void notify(){
            VariadicHelper<sizeof...(Arguments),void>::load( mRelativePaths, mCallback );
        }
        
    protected:
        std::vector<ci::fs::path>           mRelativePaths;
        std::vector<std::time_t>            mLastTimesWritten;
        std::function<void(Arguments...)>   mCallback;
    };
    
    
    
    //! VariadicLoader
    template<typename T, typename... Arguments>
    class AsyncVariadicLoader : public Loader {
    public:
        AsyncVariadicLoader( const std::vector<ci::fs::path> &relativePaths, std::function<T(Arguments...)> callback, std::function<void(T)> mainThreadCallback, const Options &options = Options() )
        : mCallback( callback ), mRelativePaths( relativePaths ), mMainThreadCallback( mainThreadCallback )
        {
            for( auto path : relativePaths ){
                mLastTimesWritten.push_back( ci::fs::last_write_time( ci::app::getAssetPath( path ) ) );
            }
            mOptions = options;
            mOptions.asynchronous();
        }
        
        
        //! Returns whether the asset needs to be reloaded or not.
        virtual bool watch(){
            bool needsReload = false;
            for( int i = 0; i < mLastTimesWritten.size(); i++ ){
                std::time_t time = ci::fs::last_write_time( ci::app::getAssetPath( mRelativePaths[i] ) );
                if( time > mLastTimesWritten[i] ){
                    mLastTimesWritten[i] = time;
                    needsReload = true;
                }
            }
            return needsReload;
        }
        
        //! Loads the asset and Notify the Callback function with the new DataSourceRef.
        virtual void notify(){
            mLoaded = false;
            std::thread( [this]() {
                ci::ThreadSetup threadSetup;
                mAsyncData = VariadicHelper<sizeof...(Arguments),T>::load( mRelativePaths, mCallback );
                mLoaded = true;
            } ).detach();
        }
        
        
        
        virtual void update()
        {
            if( loaded() ){
                if( mMainThreadCallback )
                    mMainThreadCallback( mAsyncData );
            }
        }
        
    protected:
        T                                   mAsyncData;
        std::vector<ci::fs::path>           mRelativePaths;
        std::vector<std::time_t>            mLastTimesWritten;
        std::function<T(Arguments...)>      mCallback;
        std::function<void(T)>              mMainThreadCallback;
    };
    
    typedef std::shared_ptr<Loader> LoaderRef;
    
    AssetManager(){}
    
    static AssetManager* instance();
    
    void update();
    
    std::deque< LoaderRef > mWatchingLoaders;
    std::deque< LoaderRef > mAsyncLoaders;
    static AssetManager*    mInstance;
};


//! Asynchronously loads an asset and if needed watch for hot reload.
template<typename T>
void AssetManager::load( const ci::fs::path &relativePath, std::function<T(ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options )
{
    if( ci::fs::exists( ci::app::getAssetPath( relativePath ) ) ){
        LoaderRef loader( new AsyncLoader<T>( relativePath, asyncCallback, mainThreadCallback, options ) );
        if( options.isWatching() )
            instance()->mWatchingLoaders.push_back( loader );
        instance()->mAsyncLoaders.push_back( loader );
        loader->notify();
    }
    else throw ci::app::AssetLoadExc( relativePath );
};

//! Load several assets and if needed watch for hot reload.
template<typename... Arguments>
void AssetManager::load( const std::vector<ci::fs::path> &relativePaths, std::function<void(Arguments...)> callback, const Options &options )
{
    bool filesExist = true;
    for( auto path : relativePaths ){
        filesExist = filesExist && ci::fs::exists( ci::app::getAssetPath( path ) );
        
        if( !filesExist )
            throw ci::app::AssetLoadExc( path );
    }
    if( filesExist ){
        LoaderRef loader;
        if( options.isAsynchronous() ){
            loader = LoaderRef( new AsyncVariadicLoader<void,Arguments...>( relativePaths, callback, options ) );
            instance()->mAsyncLoaders.push_back( loader );
        }
        else
            loader = LoaderRef( new VariadicLoader<Arguments...>( relativePaths, callback, options ) );
        
        if( options.isWatching() )
            instance()->mWatchingLoaders.push_back( loader );
        
        loader->notify();
    }
}

//! Asynchronously load several asset and if needed watch for hot reload.
template<typename T, typename... Arguments>
void AssetManager::load( const std::vector<ci::fs::path> &relativePaths, std::function<T(Arguments...)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options )
{
    bool filesExist = true;
    for( auto path : relativePaths ){
        filesExist = filesExist && ci::fs::exists( ci::app::getAssetPath( path ) );
        
        if( !filesExist )
            throw ci::app::AssetLoadExc( path );
    }
    if( filesExist ){
        LoaderRef loader( new AsyncVariadicLoader<T,Arguments...>( relativePaths, asyncCallback, mainThreadCallback, options ) );
        if( options.isWatching() )
            instance()->mWatchingLoaders.push_back( loader );
        instance()->mAsyncLoaders.push_back( loader );
        loader->notify();
    }
}

template<typename T>
void AssetManager::load( const ci::fs::path &p0, const ci::fs::path &p1, std::function<T(ci::DataSourceRef,ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options )
{
    load( { p0, p1 }, asyncCallback, mainThreadCallback, options );
}
template<typename T>
void AssetManager::load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, std::function<T(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options )
{
    load( { p0, p1, p2 }, asyncCallback, mainThreadCallback, options );
}
template<typename T>
void AssetManager::load( const ci::fs::path &p0, const ci::fs::path &p1, const ci::fs::path &p2, const ci::fs::path &p3, std::function<T(ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef,ci::DataSourceRef)> asyncCallback, std::function<void(T)> mainThreadCallback, const Options &options )
{
    load( { p0, p1, p2, p3 }, asyncCallback, mainThreadCallback, options );
}



//! VariadicLoader Helper End point (<0,...>)
template <typename T>
struct AssetManager::VariadicHelper<0,T>
{
    template <typename... Arguments, typename... Args >
    static T load( const std::vector<ci::fs::path> &relativePaths, const std::function<T(Arguments...)> &callback, Args... args )
    {
        return callback( args... );
    }
};

//! AsyncLoader void template specialization
template<>
class AssetManager::AsyncLoader<void> : public Loader {
public:
    AsyncLoader( ci::fs::path relativePath, std::function<void(ci::DataSourceRef)> asyncCallback, const Options &options = Options() )
    : Loader( relativePath, asyncCallback, options ) {}
    
    //! Loads the asset in a separated thread and Notify the Callback function with the new DataSourceRef.
    virtual void notify()
    {
        mLoaded = false;
        std::thread( [this]() {
            ci::ThreadSetup threadSetup;
            ci::DataSourceRef data = ci::app::loadAsset( mRelativePath );
            mCallback( data );
            mLoaded = true;
        } ).detach();
    }
};

//! AsyncVariadicLoader void template specialization
template<typename... Arguments>
class AssetManager::AsyncVariadicLoader<void,Arguments...> : public Loader {
public:
    AsyncVariadicLoader( const std::vector<ci::fs::path> &relativePaths, std::function<void(Arguments...)> callback, const Options &options = Options() )
    : mCallback( callback ), mRelativePaths( relativePaths )
    {
        for( auto path : relativePaths ){
            mLastTimesWritten.push_back( ci::fs::last_write_time( ci::app::getAssetPath( path ) ) );
        }
        mOptions = options;
        mOptions.asynchronous();
    }
    
    //! Loads the asset in a separated thread and Notify the Callback function with the new DataSourceRef.
    virtual void notify(){
        mLoaded = false;
        std::thread( [this]() {
            ci::ThreadSetup threadSetup;
            VariadicHelper<sizeof...(Arguments),void>::load( mRelativePaths, mCallback );
            mLoaded = true;
        } ).detach();
    }
    
protected:
    std::vector<ci::fs::path>           mRelativePaths;
    std::vector<std::time_t>            mLastTimesWritten;
    std::function<void(Arguments...)>   mCallback;
};