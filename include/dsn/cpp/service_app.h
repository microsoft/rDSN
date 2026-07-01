/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus (rDSN) -=- 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     application model atop zion in c++
 *
 * Revision history:
 *     Mar., 2015, @imzhenyu (Zhenyu Guo), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# pragma once

# include <dsn/service_api_c.h>
# include <dsn/cpp/auto_codes.h>
# include <dsn/cpp/address.h>
# include <vector>
# include <string>
# include <cstring>

namespace dsn 
{
    /*!
    @addtogroup app-model
    @{
    */

    class service_app
    {
    public:
        service_app(dsn_gpid gpid) 
            : _started(false), _gpid(gpid) { }

        virtual ~service_app(void) {}

        virtual ::dsn::error_code start(int argc, char** argv) = 0;

        virtual ::dsn::error_code stop(bool cleanup = false) = 0;

        //
        // inquery routines
        //
        bool is_started() const { return _started; }

        ::dsn::rpc_address primary_address() const { return _address; }

        const std::string& name() const { return _name; }

        dsn_gpid get_gpid() const { return _gpid; }
        
    private:
        bool               _started;
        ::dsn::rpc_address _address;
        std::string        _name;
        dsn_gpid           _gpid;

    public:
        template<typename TServiceApp>
        static void* app_create(const char* /*tname*/, dsn_gpid gpid)
        {
            auto svc =  new TServiceApp(gpid);
            return (void*)(dynamic_cast<service_app*>(svc));
        }

        static dsn_error_t app_start(void* app, int argc, char** argv)
        {
            service_app* sapp = (service_app*)app;
            if (sapp == nullptr)
            {
                dlog(LOG_LEVEL_ERROR, "cpp.service_app", "app_start got null app");
                return ERR_INVALID_PARAMETERS.get();
            }
            if (argc <= 0)
            {
                dlog(LOG_LEVEL_ERROR, "cpp.service_app", "app_start got invalid argc = %d", argc);
                return ERR_INVALID_PARAMETERS.get();
            }
            if (argv == nullptr)
            {
                dlog(LOG_LEVEL_ERROR, "cpp.service_app", "app_start got null argv");
                return ERR_INVALID_PARAMETERS.get();
            }
            for (int i = 0; i < argc; ++i)
            {
                if (argv[i] == nullptr)
                {
                    dlog(LOG_LEVEL_ERROR, "cpp.service_app", "app_start got null argv at index = %d", i);
                    return ERR_INVALID_PARAMETERS.get();
                }
            }

            sapp->_address = dsn_primary_address();
            sapp->_name = std::string(argv[0]);

            auto r = sapp->start(argc, argv);
            if (r == ::dsn::ERR_OK)
            {
                sapp->_started = true;
            }
            return r.get();
        }

        static dsn_error_t app_destroy(void* app, bool cleanup)
        {
            service_app* sapp = (service_app*)(app);
            if (sapp == nullptr)
            {
                dlog(LOG_LEVEL_ERROR, "cpp.service_app", "app_destroy got null app");
                return ERR_INVALID_PARAMETERS.get();
            }

            auto err = sapp->stop(cleanup);
            if (ERR_OK == err) sapp->_started = false;
            return err.get();
        }
    };

    /*! C++ wrapper of the \ref dsn_register_app function*/
    template<typename TServiceApp>
    bool register_app(const char* type_name)
    {
        if (type_name == nullptr || type_name[0] == '\0')
        {
            dlog(LOG_LEVEL_ERROR, "cpp.service_app", "register_app got invalid type_name");
            return false;
        }

        dsn_app app;
        memset(&app, 0, sizeof(app));
        app.mask = DSN_APP_MASK_APP;
        int len = snprintf(app.type_name, sizeof(app.type_name), "%s", type_name);
        if (len < 0 || static_cast<size_t>(len) >= sizeof(app.type_name))
        {
            dlog(LOG_LEVEL_ERROR, "cpp.service_app", "register_app got too long type_name: %s", type_name);
            return false;
        }
        app.layer1.create = service_app::app_create<TServiceApp>;
        app.layer1.start = service_app::app_start;
        app.layer1.destroy = service_app::app_destroy;

        return dsn_register_app(&app);
    }

    /*@}*/
} // end namespace dsn::service
