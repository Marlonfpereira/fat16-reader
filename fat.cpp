#include <iostream>
#include <cstdint>
#include <string>
#include <list>
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

        // this will be cast to it's specific type once the driver actually knows what type of FAT this is.
        unsigned char extended_section[54];
    };

    struct __attribute__((packed)) RootDirectoryEntry
    {
        unsigned char filename[11];
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

    void computeAllEntries()
    {
        for (int i = 0; true; i++)
        {
            fseek(imageFile, rootPosition + (i * 32), SEEK_SET);
            RootDirectoryEntry entry;
            fread(&entry, sizeof(RootDirectoryEntry), 1, imageFile);

            if (static_cast<int>(entry.filename[0]) == 0x00)
                break;
            cout << hex << endl
                 << rootPosition + (i * 32) << endl;
            if (static_cast<int>(entry.filename[0]) == 0xE5)
            {
                cout << "Deleted File" << endl;
                if (entry.attributes == 0x0F)
                {
                    cout << "Long Filename" << endl;
                    i++;
                }
                continue;
            }

            if (entry.attributes == 0x0F)
            {
                cout << "Long Filename" << endl;
                continue;
            }

            cout << "Filename: " << entry.filename << endl;
            cout << "Attributes: " << static_cast<int>(entry.attributes) << endl;
            cout << dec << "File Size: " << entry.file_size << " bytes" << endl;
        }
    }

    RootDirectoryEntry computeEntry(unsigned int position)
    {
        RootDirectoryEntry entry;
        for (int i = 0; i < 2; i++)
        {
            fseek(imageFile, position + (i * 32), SEEK_SET);
            fread(&entry, sizeof(RootDirectoryEntry), 1, imageFile);

            if (static_cast<int>(entry.filename[0]) == 0x00)
            {
                cout << "Entrada vazia\n";
                break;
            }
            cout << hex << endl
                 << position + (i * 32) << endl;
            if (static_cast<int>(entry.filename[0]) == 0xE5)
            {
                cout << "Deleted File" << endl;
                if (entry.attributes == 0x0F)
                    cout << "Long Filename" << endl;
                break;
            }

            if (entry.attributes == 0x0F)
            {
                cout << "Long Filename" << endl;
                continue;
            }

            cout << "Filename: " << entry.filename << endl;
            cout << "Attributes: " << static_cast<int>(entry.attributes) << endl;
            cout << dec << "File Size: " << entry.file_size << " bytes" << endl;
            return entry;
            break;
        }
        return entry;
    }

    list<int> accessFat(int position)
    {
        RootDirectoryEntry entry = computeEntry(position);
        unsigned short fatEntry = entry.cluster_low;
        list<int> fatClusters;
        do
        {
            fatClusters.push_back(fatEntry);
            fseek(imageFile, fatPosition + (fatEntry * 2), SEEK_SET);
            fread(&fatEntry, sizeof(unsigned short), 1, imageFile);
        } while (fatEntry != 0xFFFF);
        fatClusters.push_back(entry.attributes == 0x10 ? 1 : 0);
        return fatClusters;
    }

    void accessData(list<int> fatClusters)
    {
        if (fatClusters.back() == 1)
            cout << "The file is a directory" << endl;
        else
            cout << "The file is not a directory" << endl;
        fatClusters.pop_back();
        unsigned short sectorsPerCluster = static_cast<int>(boot_record.sectors_per_cluster);
        unsigned short bytesPerSector = static_cast<int>(boot_record.bytes_per_sector);
        for (auto fatCluster : fatClusters)
        {
            char buffer[bytesPerSector * sectorsPerCluster];
            fseek(imageFile, dataPosition + (((fatCluster - 2) * sectorsPerCluster)) * bytesPerSector, SEEK_SET);
            fread(buffer, bytesPerSector * sectorsPerCluster, 1, imageFile);
            cout << buffer << endl;
        }
    }

public:
    // Add methods to read the Fat16 Boot Record here

    // Constructor to read the boot record from a file
    Fat16(string filename)
    {
        imageFile = fopen(filename.c_str(), "rb");
        fseek(imageFile, 0, SEEK_SET);
        fread(&boot_record, sizeof(BootSector), 1, imageFile);

        fatPosition = boot_record.reserved_sector_count * boot_record.bytes_per_sector;
        fat2Position = fatPosition + (boot_record.table_size_16 * boot_record.bytes_per_sector);
        rootPosition = fat2Position + (boot_record.table_size_16 * boot_record.bytes_per_sector);
        dataPosition = rootPosition + (boot_record.root_entry_count * 32);
    }

    void BootRecordPrint()
    {
        cout << "bootjmp: " << boot_record.bootjmp << endl;
        cout << "oem_name: " << boot_record.oem_name << endl;
        cout << "bytes_per_sector: " << boot_record.bytes_per_sector << endl;
        cout << "sectors_per_cluster: " << static_cast<int>(boot_record.sectors_per_cluster) << endl;
        cout << "reserved_sector_count: " << boot_record.reserved_sector_count << endl;
        cout << "table_count: " << static_cast<int>(boot_record.table_count) << endl;
        cout << "root_entry_count: " << boot_record.root_entry_count << endl;
        cout << "total_sectors_16: " << boot_record.total_sectors_16 << endl;
        cout << "media_type: " << static_cast<int>(boot_record.media_type) << endl;
        cout << "table_size_16: " << boot_record.table_size_16 << endl;
        cout << "sectors_per_track: " << boot_record.sectors_per_track << endl;
        cout << "head_side_count: " << boot_record.head_side_count << endl;
        cout << "hidden_sector_count: " << boot_record.hidden_sector_count << endl;
        cout << "total_sectors_32: " << boot_record.total_sectors_32 << endl;
    }

    void positionsPrint()
    {
        cout << hex << "fatPosition: " << fatPosition << endl;
        cout << "fat2Position: " << fat2Position << endl;
        cout << "rootPosition: " << rootPosition << endl;
        cout << "dataPosition: " << dataPosition << endl;
        cout << dec;
    }

    void rootDirEntriesPrint()
    {
        cout << "/ " << endl;
        computeAllEntries();
    }

    void checkEntry()
    {
        int entry;
        cout << "Insira o endereço em hexadecimal: 0x";
        cin >> hex >> entry >> dec;
        computeEntry(entry);
    }

    void openAddress()
    {
        int entry;
        cout << "Insira o endereço em hexadecimal: 0x";
        cin >> hex >> entry >> dec;
        accessData(accessFat(entry));
    }
};

int main()
{
    Fat16 boot_record("fat16_1sectorpercluster.img");

    // boot_record.BootRecordPrint();
    // boot_record.positionsPrint();
    boot_record.rootDirEntriesPrint();
    cout << endl
         << endl;
    boot_record.openAddress();
    // boot_record.printRootDirectoryEntry();

    return 0;
}