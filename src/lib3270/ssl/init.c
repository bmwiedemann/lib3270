/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como ssl.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 *
 * References:
 *
 * http://www.openssl.org/docs/ssl/
 *
 */

/**
 * @brief OpenSSL initialization.
 *
 */

#include <config.h>
#if defined(HAVE_LIBSSL)

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>

#ifndef SSL_ST_OK
	#define SSL_ST_OK 3
#endif // !SSL_ST_OK

#include "../private.h"
#include <errno.h>
#include <lib3270.h>
#include <lib3270/internals.h>
#include <lib3270/trace.h>
#include "trace_dsc.h"

/*--[ Implement ]------------------------------------------------------------------------------------*/

/**
 * @brief Initialize openssl library.
 *
 * @return 0 if ok, non zero if fails.
 *
 */
int ssl_ctx_init(H3270 *hSession)
{
	debug("%s ssl_ctx=%p",__FUNCTION__,ssl_ctx);

	if(ssl_ctx != NULL)
		return 0;

	SSL_load_error_strings();
	SSL_library_init();

	ssl_ctx = SSL_CTX_new(SSLv23_method());
	if(ssl_ctx == NULL)
		return -1;

	SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL);
	SSL_CTX_set_info_callback(ssl_ctx, ssl_info_callback);
	SSL_CTX_set_default_verify_paths(ssl_ctx);

#if defined(_WIN32)
	{
		HKEY hKey = 0;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\" PACKAGE_NAME,0,KEY_QUERY_VALUE,&hKey) == ERROR_SUCCESS)
		{
			char			data[4096];
			unsigned long	datalen	= sizeof(data);		// data field length(in), data returned length(out)
			unsigned long	datatype;					// #defined in winnt.h (predefined types 0-11)

			if(RegQueryValueExA(hKey,"datadir",NULL,&datatype,(LPBYTE) data,&datalen) == ERROR_SUCCESS)
			{
				strncat(data,"\\certs",4095);

				trace("Loading certs from \"%s\"",data);
				if(!SSL_CTX_load_verify_locations(ssl_ctx,NULL,data))
				{
					char buffer[4096];
					int ssl_error = ERR_get_error();

					snprintf(buffer,4095,_("Cant set default locations for trusted CA certificates to\n%s"),data);

					lib3270_popup_dialog(
							hSession,
							LIB3270_NOTIFY_ERROR,
							N_( "Security error" ),
							buffer,
							N_( "%s" ),ERR_lib_error_string(ssl_error)
									);
				}
			}
			RegCloseKey(hKey);
		}


	}
#else
	static const char * ssldir[] =
	{
#ifdef DATAROOTDIR
		DATAROOTDIR "/" PACKAGE_NAME "/certs",
#endif // DATAROOTDIR
#ifdef SYSCONFDIR
		SYSCONFDIR "/ssl/certs",
		SYSCONFDIR "/certs",
#endif
		"/etc/ssl/certs"
	};

	size_t f;

	for(f = 0;f < sizeof(ssldir) / sizeof(ssldir[0]);f++)
	{
		SSL_CTX_load_verify_locations(ssl_ctx,NULL,ssldir[f]);
	}

#endif // _WIN32

	//
	// Initialize CUSTOM CRL CHECK
	//


/*
#if defined(SSL_ENABLE_CRL_CHECK)
	// Set up CRL validation
	// https://stackoverflow.com/questions/4389954/does-openssl-automatically-handle-crls-certificate-revocation-lists-now
	X509_STORE *store = SSL_CTX_get_cert_store(ssl_ctx);

	// Enable CRL checking
	X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
	X509_STORE_set1_param(store, param);
	X509_VERIFY_PARAM_free(param);

	// X509_STORE_free(store);

	trace_dsn(hSession,"CRL CHECK is enabled.\n");

#else

	trace_dsn(hSession,"CRL CHECK is disabled.\n");

#endif // SSL_ENABLE_CRL_CHECK
*/

	ssl_3270_ex_index = SSL_get_ex_new_index(0,NULL,NULL,NULL,NULL);

	return 0;
}

#endif // HAVE_LIBSSL