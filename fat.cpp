#include <iostream>
#include <cstdint>
#include <string>
using namespace std;

class Fat16BootRecord
{
private:
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

    BootSector boot_record;
    int fatPosition, fat2Position, rootPosition, dataPosition;

public:
    // Add methods to read the Fat16 Boot Record here

    // Constructor to read the boot record from a file
    Fat16BootRecord(string filename) {
        FILE *fp;
        fp = fopen(filename.c_str(), "rb");
        fseek(fp, 0, SEEK_SET);
        fread(&boot_record, sizeof(BootSector), 1, fp);

        fatPosition = boot_record.reserved_sector_count * boot_record.bytes_per_sector;
        fat2Position = fatPosition + (boot_record.table_size_16 * boot_record.bytes_per_sector);
        rootPosition = fat2Position + (boot_record.table_size_16 * boot_record.bytes_per_sector);
        dataPosition = rootPosition + (boot_record.root_entry_count * 32);
    }

    void imgPrint() {
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
    
    void positionsPrint() {
        cout << hex << "fatPosition: " << fatPosition << endl;
        cout << "fat2Position: " << fat2Position << endl;
        cout << "rootPosition: " << rootPosition << endl;
        cout << "dataPosition: " << dataPosition << endl;
        cout << dec;
    }
};

int main()
{

    FILE *fp;
    Fat16BootRecord boot_record("fat16_1sectorpercluster.img");

    boot_record.imgPrint();
    boot_record.positionsPrint();

    return 0;
}