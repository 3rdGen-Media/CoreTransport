#include "stdafx.h"
#include "ReqlSSLKey.h"

BOOL ReqlSSLGenerateKeyPair(HCRYPTPROV hCryptProv, HCRYPTKEY * hKeyPair)
{
	//---------------------------------------------------------------
    // A context with a key container is available.
    // Attempt to get the handle to the signature key. 

    // Public/private key handle.
    //HCRYPTKEY hKey = NULL;               
    
	/*
    if(CryptGetUserKey(
        hCryptProv,
        AT_SIGNATURE,
        &hKey))
    {
        printf(TEXT("A signature key is available.\n"));
    }
    else
    {
        printf(TEXT("No signature key is available.\n"));
        if(GetLastError() == NTE_NO_KEY) 
        {
            //-------------------------------------------------------
            // The error was that there is a container but no key.

            // Create a signature key pair. 
            printf(TEXT("The signature key does not exist.\n"));
            printf(TEXT("Create a signature key pair.\n")); 
            if(CryptGenKey(
                hCryptProv,
                AT_SIGNATURE,
                0,
                &hKey)) 
            {
                printf(TEXT("Created a signature key pair.\n"));
            }
            else
            {
                printf(TEXT("Error occurred creating a signature key.\n"));
				printf("**** Error 0x%x returned by CryptGenKey\n", GetLastError());
				if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
            }
        }
        else
        {
            printf(TEXT("An error other than NTE_NO_KEY getting a signature key.\n"));
			printf("**** Error 0x%x returned by CryptGenKey\n", GetLastError());
			if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
        }
    } // End if.
    //printf(TEXT("A signature key pair existed, or one was created"));


    // Destroy the signature key.
    if(hKey)
    {
        if(!(CryptDestroyKey(hKey)))
        {
            printf(TEXT("Error during CryptDestroyKey."));
        }

        hKey = NULL;
    } 
	*/

    // Next, check the exchange key. 
    if(CryptGetUserKey(
        hCryptProv,
        AT_KEYEXCHANGE,
        hKeyPair)) 
    {
        printf(TEXT("An exchange key exists.\n"));
    }
    else
    {
        printf(TEXT("No exchange key is available.\n"));

        // Check to determine whether an exchange key 
        // needs to be created.
        if(GetLastError() == NTE_NO_KEY) 
        { 
            // Create a key exchange key pair.
            printf(TEXT("The exchange key does not exist.\n"));
            printf(TEXT("Attempting to create an exchange key pair.\n"));
            if(CryptGenKey(
                hCryptProv,
                AT_KEYEXCHANGE,
                0x08000000 | CRYPT_EXPORTABLE ,
                hKeyPair)) 
            {
                printf(TEXT("Exchange key pair created.\n"));
            }
            else
            {
                printf(TEXT("Error occurred attempting to create an exchange key.\n"));
				printf("**** Error 0x%x returned by CryptGenKey\n", GetLastError());
				if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
            }
        }
        else
        {
            printf(TEXT("An error other than NTE_NO_KEY occurred.\n"));
			printf("**** Error 0x%x returned by CryptGenKey\n", GetLastError());
			if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
        }
    }

    // Destroy the exchange key.
	
    if(*hKeyPair)
    {
        if(!(CryptDestroyKey(*hKeyPair)))
        {
            printf(TEXT("Error during CryptDestroyKey."));
        }

        hKeyPair = NULL;
    }
	
	return TRUE;
}

BOOL ReqlSSLImportKey(HCRYPTPROV hProv, LPBYTE pbKeyBlob, DWORD dwBlobLen, HCRYPTKEY hPubKey)
{
    HCRYPTKEY hImportedKey;

    //---------------------------------------------------------------
    // This code assumes that a cryptographic provider (hProv) 
    // has been acquired and that a key BLOB (pbKeyBlob) that is 
    // dwBlobLen bytes long has been acquired. 

    //---------------------------------------------------------------
    // Get the public key of the user who created the digital 
    // signature and import it into the CSP by using CryptImportKey. 
    // The key to be imported is in the buffer pbKeyBlob that is  
    // dwBlobLen bytes long. This function returns a handle to the 
    // public key in hPubKey.

    if(CryptImportKey(
        hProv,
        pbKeyBlob,
        dwBlobLen,
        hPubKey,
        CRYPT_EXPORTABLE,
        &hImportedKey))
    {
        printf("The key has been imported.\n");
    }
    else
    {
        printf("Key import failed.\n");
		printf("**** Error 0x%x returned by CryptImportKey\n", GetLastError());
        if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
        return FALSE;
    }

    //---------------------------------------------------------------
    // Insert code that uses the imported public key here.
    //---------------------------------------------------------------

    //---------------------------------------------------------------
    // When you have finished using the key, you must release it.
    if(CryptDestroyKey(hImportedKey))
    {
        printf("The imported key has been released.");
    }
    else
    {
        printf("The imported key has not been released.");
        return FALSE;
    }

    return TRUE;
}

BOOL ReqlSSLExportKey(
    HCRYPTKEY * hKey, 
    DWORD dwBlobType,
    LPBYTE *ppbKeyBlob, 
    LPDWORD pdwBlobLen)
{
	//HCRYPTKEY
    DWORD dwBlobLength;
    //*ppbKeyBlob = NULL;
    *pdwBlobLen = 0;
	//hKey = NULL;
    // Export the public key. Here the public key is exported to a 
    // PUBLICKEYBLOB. This BLOB can be written to a file and
    // sent to another user.

    if(CryptExportKey(   
        *hKey,    
        0,    
        dwBlobType,
        0,    
        *ppbKeyBlob, 
        &dwBlobLength)) 
    {
        printf("Size of the BLOB for the public key determined. \n");
    }
    else
    {
        printf("Error computing BLOB length.\n");
		printf("**** Error 0x%x returned by CryptExportKey\n", GetLastError());
         if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
        return FALSE;
    }

    // Allocate memory for the pbKeyBlob.
    if(*ppbKeyBlob = (LPBYTE)malloc(dwBlobLength)) 
    {
        printf("Memory has been allocated for the BLOB. \n");
    }
    else
    {
        printf("Out of memory. \n");
        return FALSE;
    }

    // Do the actual exporting into the key BLOB.
    if(CryptExportKey(   
        *hKey, 
        0,    
        dwBlobType,    
        0,    
        *ppbKeyBlob,    
        &dwBlobLength))
    {
        printf("Contents have been written to the BLOB. \n");
        *pdwBlobLen = dwBlobLength;
    }
    else
    {
        printf("Error exporting key to BLOB.\n");
		printf("**** Error 0x%x returned by CryptExportKey\n", GetLastError());
        if( GetLastError() == CRYPT_E_NOT_FOUND ) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
        free(*ppbKeyBlob);
        *ppbKeyBlob = NULL;

        return FALSE;
    }

    return TRUE;
}
