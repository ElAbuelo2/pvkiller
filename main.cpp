#include <iostream>
#include <string>
#include <ctime>
#include <windows.h>
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

const string getCurrentDateTime ()
{
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );

    stringstream sb;
    sb << (now->tm_year + 1900) << '-'
       << (now->tm_mon + 1) << '-'
       <<  now->tm_mday << " "
       <<  now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec
       << endl;

    return sb.str();
}

namespace regedit
{
    bool DeleteValue (HKEY root_key, const wstring& key_path, const wstring& value_name)
    {
        LONG result = !ERROR_SUCCESS;
        HKEY key;

        result = RegOpenKeyEx(root_key, key_path.c_str(), 0, KEY_WRITE, &key);
        if (result != ERROR_SUCCESS)
            return false;

        // Only set value if result is ERROR_SUCCESS so far
        if (result == ERROR_SUCCESS)
        {
            result = RegDeleteValue(key, value_name.c_str());
        }

        // Clean up
        RegCloseKey(key);

        if (result == ERROR_SUCCESS)
            return true;

        return false;
    }

    bool EnumerateSubKeys (HKEY root_key, const wstring& key_path, vector<wstring>& out)
    {
        LONG result = !ERROR_SUCCESS;
        HKEY key;

        result = RegOpenKeyEx(root_key, key_path.c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &key);
        if (result != ERROR_SUCCESS)
            return false;

        // Only continue if result is ERROR_SUCCESS so far
        if (result == ERROR_SUCCESS)
        {
            int subkey_index = 0;

            while (result == ERROR_SUCCESS)
            {
                DWORD name_len = 255;
                wchar_t name [name_len] = {0};

                result = RegEnumKeyEx     ( key,
                                            subkey_index,
                                            &name[0],
                                            &name_len,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL
                                           );

                if (result == ERROR_SUCCESS)
                {
                    out.push_back(name);
                }

                subkey_index++;
            }

        }

        // Clean up
        RegCloseKey(key);

        if (result == ERROR_SUCCESS)
            return true;

        return false;
    }
}

wstring UTF8ToUnicode (const string& utf8)
{
    int len = MultiByteToWideChar ( CP_UTF8,
                                      MB_ERR_INVALID_CHARS,
                                      &utf8[0],
                                      -1,
                                      NULL,
                                      0
                                     );
    wchar_t buf [len];
    MultiByteToWideChar             ( CP_UTF8,
                                      MB_ERR_INVALID_CHARS,
                                      &utf8[0],
                                      -1,
                                      buf,
                                      len
                                     );

    return buf;
}

string UnicodeToANSI (const wstring& unicode)
{
    int len = WideCharToMultiByte   ( CP_ACP,
                                      0,
                                      &unicode[0],
                                      -1,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL
                                     );
    char buf [len];
    WideCharToMultiByte             ( CP_ACP,
                                      0,
                                      &unicode[0],
                                      -1,
                                      buf,
                                      len,
                                      NULL,
                                      NULL
                                     );

    return buf;
}

string UTF8ToANSI (const string& utf8)
{
    return UnicodeToANSI(UTF8ToUnicode(utf8));
}

void log (const string& str)
{
    wstring temp = UTF8ToUnicode(str);
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &temp[0], temp.size(), NULL, NULL);
}

void log (const wstring& str)
{
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &str[0], str.size(), NULL, NULL);
}

int main (int argc, char** argv)
{
    log(L"Este proceso no es infadible y no se acepta responsabilidad alguna por la pérdida de información valiosa.\r\n");
    log(L"Si no está de acuerdo con esto no continúe. Puede cancelar este proceso cerrando la ventana o presionando las teclas CTRL y C al mismo tiempo.\r\n");
    log(L"El programa esperará 40 segundos antes de continuar.\r\n");
    log(L"Asegúrese de que el pendrive afectado se encuentre conectado. Si no lo está, cierre esta ventana, conecte el pendrive y vuelva a ejecutar este programa.\r\n");
    log(L"Si tiene varios pendrives afectados, puede ir conectándolos de a uno y volver a ejecutar este programa por cada uno que conecta.\r\n");

    Sleep(40000);

    cout << "Comenzando escaneo a las " << getCurrentDateTime() << endl;
    if (RegDeleteValue != RegDeleteValueW)
    {
        cout << "ERROR: RegDeleteValue no es RegDeleteValueW, imposible continuar" << endl;
        return 1;
    }

    system("taskkill -f -im wscript.exe");

    cout << "Buscando en los perfiles de los usuarios" << endl;

    char current_volume = 'C';

    for (current_volume = 'C'; current_volume <= 'Z'; ++current_volume)
    {
        string paths_to_check [] = {string("") + current_volume + ":\\Documents and Settings",
                                    string("") + current_volume + ":\\Users"};

        for (int i = 0; i < 2; ++i)
        {
            path users_dir = path(paths_to_check[i]);

            //cout << users_dir << endl;

            if (!exists(users_dir))
            {
                continue;
            }

            cout << "Entrando en " << users_dir << "" << endl;

            vector<directory_entry> entries;

            try
            {
                copy(directory_iterator(users_dir), directory_iterator(), back_inserter(entries));
            }
            catch (...)
            {
                log(string("No se pudo acceder a ") + users_dir.string() + ". Tal vez es un enlace simbólico (Windows 7, 8 o 10).\r\n");
                continue;
            }

            for (vector<directory_entry>::const_iterator it = entries.begin(); it != entries.end(); ++it)
            {
                wstring locations [] = {
                                        // Windows XP (in Spanish)
                                        L"\\Configuración local\\Temp\\Doc32.wsf",
                                        L"\\Menú Inicio\\Programas\\Inicio\\Doc32.wsf",
                                        L"\\Menú Inicio\\Programas\\Inicio\\Doc32.wsf.lnk",
                                        L"\\Menú Inicio\\Programas\\Inicio\\Doc32.lnk",

                                        // Windows 7, 8, 8.1, 10 (Vista?)
                                        L"\\AppData\\Local\\Temp\\Doc32.wsf",
                                        L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Doc32.wsf",
                                        L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Doc32.wsf.lnk",
                                        L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\Doc32.lnk"
                                        };

                for (int l = 0; l < 8; ++l)
                {

                    path malfile = it->path().wstring() + locations[l];
                    if (exists(malfile))
                    {
                        log(L"Positivo en " + malfile.wstring() + L"\r\n");
                        remove(malfile);
                        if (exists(malfile))
                        {
                            cout << "ERROR: El archivo no se pudo borrar" << endl;
                        } else
                        {
                            cout << "Eliminado correctamente" << endl;
                        }
                    }

                }
            }

            cout << "Saliendo de " << users_dir << endl;
        }
    }

    cout << "Intentando eliminar los valores del registro" << endl;

    {
        vector<pair<HKEY, wstring> > keys_to_try;
        keys_to_try.push_back({HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"});
        keys_to_try.push_back({HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"});

        {
            vector<wstring> users_keys;
            regedit::EnumerateSubKeys(HKEY_USERS, L"", users_keys);

            for (size_t i = 0; i < users_keys.size(); ++i)
            {
                keys_to_try.push_back({HKEY_USERS, users_keys[i] + L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"});
            }
        }

        for (vector<pair<HKEY, wstring> >::iterator keyit = keys_to_try.begin(); keyit != keys_to_try.end(); ++keyit)
        {
            cout << "Comprobando " << keyit->second << endl;

            bool result = regedit::DeleteValue(keyit->first, keyit->second, L"Doc32");
            if (result)
            {
                cout << "Positivo en \"" << keyit->first << "\\" << keyit->second << "\"" << endl;
            }
        }
    }

    cout << "Intentando recuperar archivos reemplazados por accesos directos" << endl;

    wchar_t windir [MAX_PATH];
    GetWindowsDirectory(windir, MAX_PATH);

    current_volume = 'C';
    for (current_volume = 'C'; current_volume <= 'Z'; ++current_volume)
    {
        path malfile = string("") + current_volume + ":\\Doc32.wsf";
        if (exists(malfile))
        {
            cout << "Positivo en " << UTF8ToANSI(malfile.string()) << endl;
            remove(malfile);
            if (exists(malfile))
            {
                cout << "ERROR: El archivo no se pudo borrar" << endl;
            } else
            {
                cout << "Eliminado correctamente" << endl;
            }
        }

        // The following conditions must be met:
        //     - The volume is not the one containing the Windows folder
        //     - At least one file exists with the same name of a .lnk file

        path p(string("") + current_volume + ":\\");
        if (wstring(windir).find(UTF8ToUnicode(string("") + current_volume + ":\\")) != wstring::npos)
        {
            log(string("Se ignora la unidad ") + current_volume + ":\\ por ser la unidad de instalación de Windows\r\n");
            continue;
        }

        vector<directory_entry> entries;
        if (!exists(p))
            continue;

        //try
        //{
            copy(directory_iterator(p), directory_iterator(), back_inserter(entries));
        //}
        //catch (...)
        //{
        //    wcout << UTF8toUnicode("ERROR: No se puede acceder a uno de los archivos. Asegúrese de contar con permisos de administrador.") << endl;
        //}

        bool condition_met = false;

        vector<directory_entry>::const_iterator it = entries.begin();
        while (it != entries.end())
        {
            //try
            //{
                malfile = it->path();
            //}
            //catch (...)
            //{
            //    wcout << UTF8toUnicode("ERROR: No se puede acceder a uno de los archivos. Asegúrese de contar con permisos de administrador.") << endl;
            //    continue;
            //}

            try
            {
                if (exists(malfile) && exists(malfile.string() + ".lnk"))
                {
                    condition_met = true;
                    break;
                }
            }
            catch (...)
            {
                it++;
                continue;
            }

            it++;
        }

        if (condition_met)
        {
            cout << "La unidad " << current_volume << ": Parece estar afectada" << endl;
            log("Se intentará reparar\r\n");
            cout << "Si usted piensa que la unidad " << current_volume << ": no es un pendrive debe presionar las teclas CTRL y C ahora" << endl;
            Sleep(10000);
            system((string("del ") + current_volume + ":\\*.lnk").c_str());
            system((string("attrib -h -r -s /s /d ") + current_volume + ":\\*.*").c_str());
        }
    }

    cout << "Finalizado a las " << getCurrentDateTime() << endl;
    system("pause");
}
