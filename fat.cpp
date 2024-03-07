#include <iostream>
#include <cstdint>
#include <string>
#include <list>
#include <vector>
using namespace std;

class Fat16
{
private:
    FILE *imageFile;
    struct __attribute__((packed)) BootSector
    {
        unsigned char bootjmp[3];
        unsigned char oem_name[8];
        unsigned short bytes_per_sector;
        unsigned char sectors_per_cluster;
        unsigned short reserved_sector_count;
        unsigned char table_count;
        unsigned short root_entry_count;
        unsigned short total_sectors_16;
        unsigned char media_type;
        unsigned short table_size_16;
        unsigned short sectors_per_track;
        unsigned short head_side_count;
        unsigned int hidden_sector_count;
        unsigned int total_sectors_32;
        unsigned char extended_section[54];
    };

    struct __attribute__((packed)) DirectoryEntry
    {
        unsigned char filename[8];
        unsigned char extension[3];
        unsigned char attributes;
        unsigned char reserved;
        unsigned char creation_time_tenths;
        unsigned short creation_time;
        unsigned short creation_date;
        unsigned short access_date;
        unsigned short cluster_high;
        unsigned short modification_time;
        unsigned short modification_date;
        unsigned short cluster_low;
        unsigned int file_size;
    };

    BootSector boot_record;
    int fatPosition, fat2Position, rootPosition, dataPosition;
    int manual = 0;
    vector<unsigned int> currentEntries;

    void printEntry(DirectoryEntry entry)
    {
        cout << "Nome: ";
        for (auto c : entry.filename)
            if (c != ' ')
                cout << c;
            else
                break;

        if (static_cast<int>(entry.attributes) == 0x10)
            cout << "\nTipo: "
                 << "Diretorio" << endl;

        else
        {
            cout << '.' << entry.extension << endl;
            cout << "Tipo: "
                 << "Arquivo" << endl;
        }
        cout << "Primeiro cluster: 0x" << hex << entry.cluster_low << dec << endl;
        cout << dec << "Tamanho: " << entry.file_size << " bytes" << endl
             << endl;
    }

    void computeAllEntries(int position)
    {
        unsigned short count = 0;
        currentEntries.clear();
        for (int i = 0; true; i++)
        {
            fseek(imageFile, position + (i * 32), SEEK_SET);
            DirectoryEntry entry;
            fread(&entry, sizeof(DirectoryEntry), 1, imageFile);

            if (static_cast<int>(entry.filename[0]) == 0x00)
                break;
            if (static_cast<int>(entry.filename[0]) == 0xE5)
            {
                if (entry.attributes == 0x0F)
                {
                    i++;
                }
                continue;
            }

            if (!manual && static_cast<int>(entry.filename[0]) == 0x2E)
            {
                continue;
            }

            if (entry.attributes == 0x0F)
            {
                continue;
            }

            cout << ++count << "-";
            currentEntries.push_back(position + (i * 32));
            if (manual)
                cout << hex << "0x" << position + (i * 32) << endl;

            printEntry(entry);
        }
    }

    DirectoryEntry computeEntry(unsigned int position)
    {
        DirectoryEntry entry;
        for (int i = 0; i < 2; i++)
        {
            fseek(imageFile, position + (i * 32), SEEK_SET);
            fread(&entry, sizeof(DirectoryEntry), 1, imageFile);

            if (static_cast<int>(entry.filename[0]) == 0x00)
            {
                cout << "Entrada vazia\n";
                break;
            }
            if (manual)
                cout << hex << position + (i * 32) << endl;

            if (static_cast<int>(entry.filename[0]) == 0xE5)
            {
                cout << "Entrada excluída" << endl;
                break;
            }

            if (entry.attributes == 0x0F)
            {
                cout << "Long Filename" << endl;
                break;
            }

            cout << "\nAcessando:\n";
            printEntry(entry);
            return entry;
            break;
        }
        return entry;
    }

    list<unsigned int> accessFat(int position)
    {
        DirectoryEntry entry = computeEntry(position);
        if (static_cast<int>(entry.filename[0]) == 0x00)
            return list<unsigned int>();

        unsigned short fatEntry = entry.cluster_low;
        list<unsigned int> fatClusters;
        if (fatEntry == 0)
            fatClusters.push_back(fatEntry);
        else
            do
            {
                fatClusters.push_back(fatEntry);
                fseek(imageFile, fatPosition + (fatEntry * 2), SEEK_SET);
                fread(&fatEntry, sizeof(unsigned short), 1, imageFile);
            } while (fatEntry != 0xFFFF);

        fatClusters.push_back(entry.attributes == 0x10 ? 1 : 0);
        fatClusters.push_back(entry.file_size);
        return fatClusters;
    }

    unsigned int accessData(list<unsigned int> fatClusters)
    {
        if (fatClusters.empty())
            return 0;
        cout << "Conteudo: \n";
        unsigned int size = fatClusters.back();
        fatClusters.pop_back();
        unsigned int isDir = fatClusters.back();
        fatClusters.pop_back();
        if (isDir == 1)
        {
            unsigned short sectorsPerCluster = static_cast<int>(boot_record.sectors_per_cluster);
            unsigned short bytesPerSector = static_cast<int>(boot_record.bytes_per_sector);
            for (auto fatCluster : fatClusters)
            {
                if (fatCluster == 0)
                {
                    rootDirEntriesPrint();
                    break;
                }
                computeAllEntries(dataPosition + (((fatCluster - 2) * sectorsPerCluster)) * bytesPerSector);
            }
        }
        else
        {
            unsigned short sectorsPerCluster = static_cast<int>(boot_record.sectors_per_cluster);
            unsigned short bytesPerSector = static_cast<int>(boot_record.bytes_per_sector);
            unsigned short bluster = sectorsPerCluster * bytesPerSector;
            unsigned int currentSize = 0;
            for (auto fatCluster : fatClusters)
            {
                fseek(imageFile, dataPosition + (((fatCluster - 2) * sectorsPerCluster)) * bytesPerSector, SEEK_SET);
                unsigned int clusterSize = 0;
                do
                {
                    char byte;
                    fread(&byte, sizeof(char), 1, imageFile);
                    cout << byte;
                    currentSize++;
                    clusterSize++;
                } while (currentSize < size && clusterSize < bluster);
            }
            cout << endl;
        }
        return isDir;
    }

public:
    Fat16(string filename, string mode)
    {
        imageFile = fopen(filename.c_str(), "rb");
        if (!imageFile)
            throw runtime_error("Nao foi possivel abrir o arquivo: " + filename);

        fseek(imageFile, 0, SEEK_SET);
        fread(&boot_record, sizeof(BootSector), 1, imageFile);
        if (mode == "manual")
            manual = 1;

        fatPosition = boot_record.reserved_sector_count * boot_record.bytes_per_sector;
        fat2Position = fatPosition + (boot_record.table_size_16 * boot_record.bytes_per_sector);
        rootPosition = fat2Position + (boot_record.table_size_16 * boot_record.bytes_per_sector);
        dataPosition = rootPosition + (boot_record.root_entry_count * 32);
    }

    void BootRecordPrint()
    {
        cout << "\nBoot Record:\n";
        cout << "-bootjmp: " << boot_record.bootjmp[0] << boot_record.bootjmp[1] << boot_record.bootjmp[2] << endl;
        cout << "-oem_name: " << boot_record.oem_name << endl;
        cout << "-bytes_per_sector: " << boot_record.bytes_per_sector << endl;
        cout << "-sectors_per_cluster: " << static_cast<int>(boot_record.sectors_per_cluster) << endl;
        cout << "-reserved_sector_count: " << boot_record.reserved_sector_count << endl;
        cout << "-table_count: " << static_cast<int>(boot_record.table_count) << endl;
        cout << "-root_entry_count: " << boot_record.root_entry_count << endl;
        cout << "-total_sectors_16: " << boot_record.total_sectors_16 << endl;
        cout << "-media_type: " << static_cast<int>(boot_record.media_type) << endl;
        cout << "-table_size_16: " << boot_record.table_size_16 << endl;
        cout << "-sectors_per_track: " << boot_record.sectors_per_track << endl;
        cout << "-head_side_count: " << boot_record.head_side_count << endl;
        cout << "-hidden_sector_count: " << boot_record.hidden_sector_count << endl;
        cout << "-total_sectors_32: " << boot_record.total_sectors_32 << endl;
    }

    void positionsPrint()
    {
        cout << "\nPosicoes:\n";
        cout << hex << "-fatPosition: 0x" << fatPosition << endl;
        cout << "-fat2Position: 0x" << fat2Position << endl;
        cout << "-rootPosition: 0x" << rootPosition << endl;
        cout << "-dataPosition: 0x" << dataPosition << endl;
        cout << dec;
    }

    unsigned int rootDirEntriesPrint()
    {
        cout << "\nDiretorio raiz:\n";
        computeAllEntries(rootPosition);
        return rootPosition;
    }

    void checkEntry()
    {
        int entry;
        cout << "Insira o endereco em hexadecimal: 0x";
        cin >> hex >> entry >> dec;
        computeEntry(entry);
    }

    void openAddress(int position)
    {
        accessData(accessFat(position));
    }

    void menu()
    {
        unsigned int op, rootDir = rootDirEntriesPrint();
        unsigned int currentDir = rootDir;
        do
        {
            if (currentDir == rootDir)
            {
                cout << "Acessar entrada: (0 para sair)\n";
                cin >> op;
                if (!op)
                    break;
            }
            else
            {
                cout << "Acessar entrada: (0 para retornar)\n";
                cin >> op;
                if (!op)
                {
                    currentDir = rootDir;
                    rootDirEntriesPrint();
                    cout << "Retornando..." << endl;
                    continue;
                }
            }

            unsigned int selected = currentEntries[op - 1];
            if (!accessData(accessFat(selected))) {
                rootDirEntriesPrint();
                currentDir = rootDir;
            }
            else
                currentDir = selected;

        } while (true);
    }

    void printFirstFile()
    {
        for (int i = 0; true; i++)
        {
            fseek(imageFile, rootPosition + (i * 32), SEEK_SET);
            DirectoryEntry entry;
            fread(&entry, sizeof(DirectoryEntry), 1, imageFile);
            if (entry.attributes == 0x20 && static_cast<int>(entry.filename[0]) != 0xE5)
            {
                accessData(accessFat(rootPosition + (i * 32)));
                return;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    string mode;
    if (argc > 1)
        mode = argv[1];
    string filename;
    cout << "Insira o caminho da imagem: ";
    cin >> filename;
    Fat16 boot_record(filename, mode);

    boot_record.BootRecordPrint();
    boot_record.printFirstFile();

    if (mode == "manual")
    {
        int op = 1;
        while (op)
        {
            cout << "\n-----------------------\n";
            cout << "0 - Finalizar\n"
                 << "1 - Entradas do diretorio raiz\n"
                 << "2 - Acessar endereco\n"
                 << "3 - Boot Record\n"
                 << "4 - Posicoes\n";
            cout << "-----------------------\n";
            cin >> dec >> op;
            switch (op)
            {
            case 0:
                return 0;
            case 1:
                boot_record.rootDirEntriesPrint();
                break;
            case 2:
                int position;
                cout << "Insira o endereco em hexadecimal: 0x";
                cin >> hex >> position >> dec;
                boot_record.openAddress(position);
                break;
            case 3:
                boot_record.BootRecordPrint();
                break;
            case 4:
                boot_record.positionsPrint();
                break;
            }
        }
    }
    else
    {
        boot_record.menu();
    }

    return 0;
}
