#ifndef UTILITIES_H
#define UTILITIES_H

    #include "fat32.h"
    #include <iostream>
    #include <iostream>
    #include <cstdint>
    #include <cstring>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <vector>
    #include <time.h>
    using namespace std;

    string month[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    int image;

    BPB_struct bpb;
    BPB32_struct bpb32;
    uint32_t* FAT;

    uint32_t current_dir_number;
    uint32_t target1_dir_number;
    uint32_t target2_dir_number;

    uint8_t* current_cluster_head;
    uint8_t* target1_cluster_head;
    uint8_t* target2_cluster_head;

    vector<uint32_t> current_cluster_chain;
    vector<uint32_t> target1_cluster_chain;
    vector<uint32_t> target2_cluster_chain;
    
    int data_start;
    int FAT_start;

    typedef struct struct_entry{
        char* name;
        bool is_dir;
        uint8_t attributes;
        uint8_t createTimeMs;
        int creationTime[3];
        int creationDate[3];
        int lastAccessTime[3];
        int modifiedTime[3];
        int modifiedDate[3];
        uint16_t eaIndex;
        uint16_t firstCluster;
        uint32_t fileSize;
    } sentry;

    void init(const char* img);
    void read_cluster(uint32_t cluster_number, uint8_t* target_cluster);
    uint32_t get_cluster_offset(uint32_t cluster_number);
    vector<uint32_t> get_chain(uint32_t first_cluster);
    void parse_input(string* _entry, vector<string>* _command);
    vector<sentry> generate_entry(uint8_t* cluster);
    void print_entries(vector<sentry>* current_entries);
    vector<pair<string,pair<uint32_t,bool>>> list_entry_names(uint32_t first_cluster);
    vector<sentry> list_entry_props(uint32_t first_cluster);
    string retrieve_path(vector<string> pv);
    vector<string> find_path_vector(string path);
    pair<uint32_t,bool> find_path(string path);
    vector<FatFileEntry> generate_data_entry(string name, bool is_dir, uint32_t root_cluster_number, uint32_t allocated_number, int entry_index);
    int find_empty_FAT_entry();
    void print_cluster(uint8_t* a, int b);

    void init(const char* img){
        image = open(img, O_RDWR);
        lseek(image, 0, SEEK_SET);
        read(image, &bpb, sizeof(bpb));

        bpb32 = bpb.extended;
        FAT_start = bpb.ReservedSectorCount*BPS;
        data_start = FAT_start+bpb32.FATSize*bpb.NumFATs*BPS;

        FAT = new uint32_t[bpb32.FATSize*BPS/4];
        lseek(image, FAT_start, SEEK_SET);
        read(image, FAT, bpb32.FATSize*BPS);

        current_dir_number = bpb32.RootCluster;
        current_cluster_head = new uint8_t[bpb.SectorsPerCluster*BPS];
        target1_cluster_head = new uint8_t[bpb.SectorsPerCluster*BPS];
        target2_cluster_head = new uint8_t[bpb.SectorsPerCluster*BPS];
        read_cluster(current_dir_number, current_cluster_head);

        current_cluster_chain = get_chain(current_dir_number);
    }

    void read_cluster(uint32_t cluster_number, uint8_t* target_cluster){
        lseek(image, get_cluster_offset(cluster_number), SEEK_SET);
        read(image, target_cluster, bpb.SectorsPerCluster*BPS);
    }

    uint32_t get_cluster_offset(uint32_t cluster_number){
        if(cluster_number<0) return -1;
        return data_start+(cluster_number-2)*(bpb.SectorsPerCluster*BPS); //some bugs maybe
    }

    vector<uint32_t> get_chain(uint32_t first_cluster){
        vector<uint32_t>  target_chain;
        target_chain.push_back(first_cluster);

        uint32_t tmp_point = first_cluster;
        while((FAT[(int)tmp_point] ^ 0x0ffffff8)){
            target_chain.push_back(FAT[(int)tmp_point]);
            tmp_point = FAT[(int)tmp_point];
        }

        return target_chain;
    }

    void parse_input(string* _entry, vector<string>* _command){
        size_t pos = _entry->size();
        _command->clear();
        while((pos = _entry->find(" ")) != string::npos){
            _command->push_back(_entry->substr(0,pos));
            _entry->erase(0, pos+1);
        }
        _command->push_back(*_entry);
    }

    vector<sentry> generate_entry(uint8_t* cluster){
        vector<sentry> current_entries;
        struct_FatFileEntry* tmp = (struct_FatFileEntry*) cluster;
        int i=0;
        string stmp = "";
        while(1){
            if(!tmp[i].lfn.sequence_number) break;
            if(!(tmp[i].lfn.attributes ^ 0x0f) && !(tmp[i].lfn.reserved) && !(tmp[i].lfn.firstCluster)){
                string sstmp = "";
                for(int j=0;j<5;j++){
                    if((tmp[i].lfn.name1[j]) != ((uint16_t) -1)) sstmp+=(wchar_t)tmp[i].lfn.name1[j];
                }
                for(int j=0;j<6;j++){
                    if(tmp[i].lfn.name2[j] != ((uint16_t) -1)) sstmp+=(wchar_t)tmp[i].lfn.name2[j];
                }
                for(int j=0;j<2;j++){
                    if(tmp[i].lfn.name3[j] != ((uint16_t) -1)) sstmp+=(wchar_t)tmp[i].lfn.name3[j];
                }
                stmp = sstmp+stmp;
            }
            else{ 
                sentry new_entry;
                if(!stmp.size()){
                    for(int ii=0;ii<11;ii++){if(tmp[i].msdos.filename[ii] != 0x20) stmp += tmp[i].msdos.filename[ii];} 
                }
                new_entry.name = new char[stmp.size()];
                strcpy(new_entry.name,stmp.c_str());
                new_entry.is_dir = (tmp[i].msdos.attributes^0x20);
                new_entry.attributes = tmp[i].msdos.attributes;
                new_entry.createTimeMs = tmp[i].msdos.creationTimeMs;
                new_entry.creationTime[2] = tmp[i].msdos.creationTime & 0x001f;
                new_entry.creationTime[1] = (tmp[i].msdos.creationTime & 0x07d0) >> 5;
                new_entry.creationTime[0] = (tmp[i].msdos.creationTime & 0xf800) >> 11;

                new_entry.creationDate[2] = tmp[i].msdos.creationDate & 0x001f;
                new_entry.creationDate[1] = 1+((tmp[i].msdos.creationDate & 0x01e0) >> 5);
                new_entry.creationDate[0] = 1980+((tmp[i].msdos.creationDate & 0xfe00) >> 9);

                new_entry.lastAccessTime[2] = tmp[i].msdos.lastAccessTime & 0x001f;
                new_entry.lastAccessTime[1] = (tmp[i].msdos.lastAccessTime & 0x07d0) >> 5;
                new_entry.lastAccessTime[0] = (tmp[i].msdos.lastAccessTime & 0xf800) >> 11;

                new_entry.modifiedTime[2] = tmp[i].msdos.modifiedTime & 0x001f;
                new_entry.modifiedTime[1] = (tmp[i].msdos.modifiedTime & 0x07d0) >> 5;
                new_entry.modifiedTime[0] = (tmp[i].msdos.modifiedTime & 0xf800) >> 11;

                new_entry.modifiedDate[2] = tmp[i].msdos.modifiedDate & 0x001f;
                new_entry.modifiedDate[1] = 1+((tmp[i].msdos.modifiedDate & 0x01e0) >> 5);
                new_entry.modifiedDate[0] = 1980+((tmp[i].msdos.modifiedDate & 0xfe00) >> 9);

                new_entry.eaIndex = tmp[i].msdos.eaIndex;
                new_entry.firstCluster = tmp[i].msdos.firstCluster;
                new_entry.fileSize = tmp[i].msdos.fileSize;
                current_entries.push_back(new_entry);

                stmp = "";
            }
            i++;
        }
        return current_entries;
    }

    vector<sentry> generate_entry_raw(struct_FatFileEntry* tmp){
        vector<sentry> current_entries;
        int i=0;
        string stmp = "";
        while(1){
            if(!tmp[i].lfn.sequence_number) break;
            if(!(tmp[i].lfn.attributes ^ 0x0f) && !(tmp[i].lfn.reserved) && !(tmp[i].lfn.firstCluster)){
                string sstmp = "";
                for(int j=0;j<5;j++){
                    if((tmp[i].lfn.name1[j]) != ((uint16_t) -1)) sstmp+=(wchar_t)tmp[i].lfn.name1[j];
                }
                for(int j=0;j<6;j++){
                    if(tmp[i].lfn.name2[j] != ((uint16_t) -1)) sstmp+=(wchar_t)tmp[i].lfn.name2[j];
                }
                for(int j=0;j<2;j++){
                    if(tmp[i].lfn.name3[j] != ((uint16_t) -1)) sstmp+=(wchar_t)tmp[i].lfn.name3[j];
                }
                stmp = sstmp+stmp;
            }
            else{ 
                sentry new_entry;
                if(!stmp.size()){
                    for(int ii=0;ii<11;ii++){if(tmp[i].msdos.filename[ii] != 0x20) stmp += tmp[i].msdos.filename[ii];} 
                }
                new_entry.name = new char[stmp.size()];
                strcpy(new_entry.name,stmp.c_str());
                new_entry.is_dir = (tmp[i].msdos.attributes^0x20);
                new_entry.attributes = tmp[i].msdos.attributes;
                new_entry.createTimeMs = tmp[i].msdos.creationTimeMs;
                new_entry.creationTime[2] = tmp[i].msdos.creationTime & 0x001f;
                new_entry.creationTime[1] = (tmp[i].msdos.creationTime & 0x07d0) >> 5;
                new_entry.creationTime[0] = (tmp[i].msdos.creationTime & 0xf800) >> 11;

                new_entry.creationDate[2] = tmp[i].msdos.creationDate & 0x001f;
                new_entry.creationDate[1] = 1+((tmp[i].msdos.creationDate & 0x01e0) >> 5);
                new_entry.creationDate[0] = 1980+((tmp[i].msdos.creationDate & 0xfe00) >> 9);

                new_entry.lastAccessTime[2] = tmp[i].msdos.lastAccessTime & 0x001f;
                new_entry.lastAccessTime[1] = (tmp[i].msdos.lastAccessTime & 0x07d0) >> 5;
                new_entry.lastAccessTime[0] = (tmp[i].msdos.lastAccessTime & 0xf800) >> 11;

                new_entry.modifiedTime[2] = tmp[i].msdos.modifiedTime & 0x001f;
                new_entry.modifiedTime[1] = (tmp[i].msdos.modifiedTime & 0x07d0) >> 5;
                new_entry.modifiedTime[0] = (tmp[i].msdos.modifiedTime & 0xf800) >> 11;

                new_entry.modifiedDate[2] = tmp[i].msdos.modifiedDate & 0x001f;
                new_entry.modifiedDate[1] = 1+((tmp[i].msdos.modifiedDate & 0x01e0) >> 5);
                new_entry.modifiedDate[0] = 1980+((tmp[i].msdos.modifiedDate & 0xfe00) >> 9);

                new_entry.eaIndex = tmp[i].msdos.eaIndex;
                new_entry.firstCluster = tmp[i].msdos.firstCluster;
                new_entry.fileSize = tmp[i].msdos.fileSize;
                current_entries.push_back(new_entry);

                stmp = "";
            }
            i++;
        }
        return current_entries;
    }

    vector<pair<string,pair<uint32_t,bool>>> list_entry_names(uint32_t first_cluster){
        vector<pair<string,pair<uint32_t,bool>>> result;
        vector<u_int32_t> cluster_chain = get_chain(first_cluster);
        uint8_t* new_target = new uint8_t[cluster_chain.size()*bpb.SectorsPerCluster*BPS];
        for(int i=0;i<cluster_chain.size();i++){
            read_cluster(cluster_chain[i], target1_cluster_head);
            for(int j=0;j<bpb.SectorsPerCluster*BPS;j++){
                new_target[i*bpb.SectorsPerCluster*BPS+j] = target1_cluster_head[j];
            }
        }
        vector<sentry> tmp = generate_entry(new_target);
        for(int j=0;j<tmp.size();j++){
            result.push_back({tmp[j].name,{((tmp[j].eaIndex << 16)+tmp[j].firstCluster),tmp[j].is_dir}});
        }
        tmp.clear();

        delete[] new_target;

        return result;
    }

    vector<sentry> list_entry_props(uint32_t first_cluster){
        vector<sentry> result;
        vector<u_int32_t> cluster_chain = get_chain(first_cluster);
        uint8_t* new_target = new uint8_t[cluster_chain.size()*bpb.SectorsPerCluster*BPS];
        for(int i=0;i<cluster_chain.size();i++){
            read_cluster(cluster_chain[i], target1_cluster_head);
            for(int j=0;j<bpb.SectorsPerCluster*BPS;j++){
                new_target[i*bpb.SectorsPerCluster*BPS+j] = target1_cluster_head[j];
            }
        }
        vector<sentry> tmp = generate_entry(new_target);
        for(int j=0;j<tmp.size();j++){
            result.push_back(tmp[j]);
        }
        tmp.clear();
        delete[] new_target;

        return result;
    }
    
    string retrieve_path(vector<string> pv){
        string tmp = "/";
        for (int i=0;i<pv.size();i++){
            if(pv[i] == "/" && i==0) continue;
            else tmp+=pv[i];
        }
        
        return tmp;
    }

    vector<string> find_path_vector(string path){
        vector<string> res;
        size_t pos = path.size();
        while((pos = path.find("/")) != string::npos){
            if(!pos) res.push_back("/");
            else res.push_back(path.substr(0,pos));
            path.erase(0, pos+1);
        }
        res.push_back(path);

        return res;
    }

    pair<uint32_t,bool> find_path(string path){
        vector<string> path_vector = find_path_vector(path);
        vector<pair<string,pair<uint32_t,bool>>> list;
        uint32_t lookup = current_dir_number;
        bool is_find = true;
        bool is_dir = false;

        for(int i=0;i<path_vector.size();i++){
            is_dir = false;
            if(path_vector[i] == "/" && path_vector[1] == "\0") return {bpb32.RootCluster,true};
            list = list_entry_names(lookup);
            is_find = false;
            if(path_vector[i] == "/"){ lookup=bpb32.RootCluster; list = list_entry_names(lookup); i++;}
            for(int j=0;j<list.size();j++){
                if(list[j].first == path_vector[i]){
                    is_find = true;
                    if(list[j].second.second) is_dir=true;
                    lookup = list[j].second.first;
                    break;
                }
            }
            if((path_vector[i] == ".") && (lookup == bpb32.RootCluster)){ is_find = true; is_dir=true;}
            if(!is_find) return {-1,false};
            if(!lookup) lookup = bpb32.RootCluster;
        }
        
        return {lookup,is_dir};
    }

    void mkdir_helper(uint32_t root_cluster_number, uint32_t allocated_number){
        uint8_t folder_specs[64];
        
        time_t rawtime;
        struct tm * ct;
        time(&rawtime);
        ct = gmtime(&rawtime);
        if(root_cluster_number != bpb32.RootCluster){
            u_int8_t tmpcluster[bpb.SectorsPerCluster*BPS];
            read_cluster(root_cluster_number, tmpcluster);
            // cout << "the cluster\n";
            // print_cluster(tmpcluster);
            ((FatFile83*)folder_specs)[1] = ((FatFile83*)tmpcluster)[0];
            ((FatFile83*)folder_specs)[1].filename[1] = '.';
        }
        else{
            for(int i=0;i<11;i++)((FatFile83*)folder_specs)[1].filename[i] = 0x20;
            ((FatFile83*)folder_specs)[1].filename[0] = '.';
            ((FatFile83*)folder_specs)[1].filename[1] = '.';
            ((FatFile83*)folder_specs)[1].attributes = 0x10;
            ((FatFile83*)folder_specs)[1].reserved = 0x0;
            ((FatFile83*)folder_specs)[1].fileSize = 0x0;
            ((FatFile83*)folder_specs)[1].creationTimeMs = 0x0;
            ((FatFile83*)folder_specs)[1].creationTime = 0x0;
            ((FatFile83*)folder_specs)[1].creationDate = 0x0;
            ((FatFile83*)folder_specs)[1].lastAccessTime = 0x0;
            ((FatFile83*)folder_specs)[1].eaIndex = bpb32.RootCluster >> 16;
            ((FatFile83*)folder_specs)[1].modifiedTime = 0x0;
            ((FatFile83*)folder_specs)[1].modifiedDate = 0x0;
            ((FatFile83*)folder_specs)[1].firstCluster = bpb32.RootCluster;
        }
        for(int i=0;i<11;i++)((FatFile83*)folder_specs)[0].filename[i] = 0x20;
        ((FatFile83*)folder_specs)[0].filename[0] = '.';
        ((FatFile83*)folder_specs)[0].attributes = 0x10;
        ((FatFile83*)folder_specs)[0].reserved = 0x0;
        ((FatFile83*)folder_specs)[0].fileSize = 0x0;
        ((FatFile83*)folder_specs)[0].creationTimeMs = 0x0;
        ((FatFile83*)folder_specs)[0].creationTime = (uint16_t)((ct->tm_hour << 11)+(ct->tm_min << 5)+(ct->tm_sec/2));
        ((FatFile83*)folder_specs)[0].creationDate = (((ct->tm_year-80) << 9)+((ct->tm_mon) << 5)+(ct->tm_mday));
        ((FatFile83*)folder_specs)[0].lastAccessTime = ((FatFile83*)folder_specs)[0].creationTime;
        ((FatFile83*)folder_specs)[0].eaIndex = allocated_number >> 16;
        ((FatFile83*)folder_specs)[0].modifiedTime = ((FatFile83*)folder_specs)[0].creationTime;
        ((FatFile83*)folder_specs)[0].modifiedDate = ((FatFile83*)folder_specs)[0].creationDate;
        ((FatFile83*)folder_specs)[0].firstCluster = allocated_number;
        lseek(image, get_cluster_offset(allocated_number), SEEK_SET);
        write(image, folder_specs, 64);
    }

    vector<struct_FatFileEntry> generate_data_entry(string name, bool is_dir, uint32_t root_cluster_number, uint32_t allocated_number, int entry_index){
        vector<struct_FatFileEntry> result;
        if(is_dir) mkdir_helper(root_cluster_number, allocated_number);
        
        vector<string> substrs;
        int i=0;
        string bucket = "";
        while(i<name.size()+1){

            if((!(i%13) && i>1) || i == name.size()) {substrs.push_back(bucket); bucket = "";}
            bucket+=name[i];    
            i++;       
        }

        string entry_index_str = to_string(entry_index+1);

        int chck = 0;                                                                                                                                                                                           

        for(int i=substrs.size()-1;i>-1;i--){
            struct_FatFileEntry* tmp = new struct_FatFileEntry;
            string sstmp = substrs[i];

            for(int i=0;i<5;i++) tmp->lfn.name1[i] = 0xffff;
            for(int i=0;i<6;i++) tmp->lfn.name2[i] = 0xffff;
            for(int i=0;i<2;i++) tmp->lfn.name3[i] = 0xffff;
             
            for(int j=0;j<(sstmp.size()>5?5:sstmp.size());j++){
                tmp->lfn.name1[j] = (uint16_t)(sstmp[j]);
            }

            sstmp.erase(0,(sstmp.size()>5?5:sstmp.size()));

            for(int j=0;j<(sstmp.size()>6?6:sstmp.size());j++){
                tmp->lfn.name2[j] = (uint16_t)(sstmp[j]);
            }

            sstmp.erase(0,(sstmp.size()>6?6:sstmp.size()));

            for(int j=0;j<(sstmp.size()>2?2:sstmp.size());j++){
                tmp->lfn.name3[j] = (uint16_t)(sstmp[j]);
            }

            sstmp.erase(0,(sstmp.size()>2?2:sstmp.size()));

            if(i==substrs.size()-1) tmp->lfn.sequence_number = 0x40+substrs.size();
            else tmp->lfn.sequence_number = i+1;
            tmp->lfn.attributes = 0x0f;
            tmp->lfn.firstCluster = 0x0000;
            tmp->lfn.reserved = 0x00;

            tmp->lfn.checksum = chck;

            result.push_back(*tmp);
            delete tmp;
        }

        time_t rawtime;
        struct tm * ct;
        time(&rawtime);
        ct = gmtime(&rawtime);

        struct_FatFileEntry sfn;
        for(int iii=0;iii<8;iii++) sfn.msdos.filename[iii] = 0x20;
        for(int iii=0;iii<3;iii++) sfn.msdos.extension[iii] = 0x20;
        sfn.msdos.filename[0] = 0x7e;
        for(int iii=1;iii<entry_index_str.size()+1&&iii<11;iii++) sfn.msdos.filename[iii] = entry_index_str[iii-1];
        if(is_dir) sfn.msdos.attributes = 0x10;
        else sfn.msdos.attributes = 0x20;
        sfn.msdos.fileSize = 0x0;
        sfn.msdos.reserved = 0x0;
        sfn.msdos.creationTimeMs = 0x0;
        sfn.msdos.creationTime = (uint16_t)((ct->tm_hour << 11)+(ct->tm_min << 5)+(ct->tm_sec/2));
        sfn.msdos.creationDate = (((ct->tm_year-80) << 9)+((ct->tm_mon) << 5)+(ct->tm_mday));
        sfn.msdos.lastAccessTime = sfn.msdos.creationTime;
        sfn.msdos.eaIndex = allocated_number >> 16;
        sfn.msdos.modifiedTime = sfn.msdos.creationTime;
        sfn.msdos.modifiedDate = sfn.msdos.creationDate;
        sfn.msdos.firstCluster = allocated_number;
        sfn.msdos.fileSize = 0x0;
        result.push_back(sfn);
        return result;

    }

    int find_empty_FAT_entry(){
        for(int i=0;i<bpb32.FATSize*BPS/4;i++){
            if(!FAT[i]) return i;
        }
        return -1;
    }

    int find_empty_data_entry(vector<uint32_t> cluster_chain){
        uint8_t tmp_cluster[bpb.SectorsPerCluster*BPS];
        read_cluster(cluster_chain[cluster_chain.size()-1],tmp_cluster);
        for(int i=0;i<bpb.SectorsPerCluster*BPS/sizeof(FatFileEntry);i++){
            if(!(((FatFileEntry*) tmp_cluster)[i].lfn.sequence_number)) return i;
        }
        return -1;
    }

    int get_tilda_index(uint32_t first_cluster){
        vector<u_int32_t> cluster_chain = get_chain(first_cluster);
        uint8_t* new_target = new uint8_t[cluster_chain.size()*bpb.SectorsPerCluster*BPS];
        for(int i=0;i<cluster_chain.size();i++){
            read_cluster(cluster_chain[i], target1_cluster_head);
            for(int j=0;j<bpb.SectorsPerCluster*BPS;j++){
                new_target[i*bpb.SectorsPerCluster*BPS+j] = target1_cluster_head[j];
            }
        }
        int result = 0;
        for(int i=0;i<cluster_chain.size()*bpb.SectorsPerCluster*BPS/32;i++){
            if(((FatFileEntry*)new_target)[i].lfn.sequence_number == 0x7e) result++;
        }

        delete[] new_target;
        return result;
    }

    /*---------------------------------PRINT---------------------------------*/
    void print_bpb32(BPB32_struct* bs){
        cout << "\t*******************************************************" << endl;
        printf("\tFATSize: %d\n", (int)bs->FATSize);
        printf("\tExtFlags: %d\n", (int)bs->ExtFlags);
        printf("\tFSVersion: %d\n", (int)bs->FSVersion);
        printf("\tRootCluster: %d\n", (int)bs->RootCluster);
        printf("\tFSInfo: %d\n", (int)bs->FSInfo);
        printf("\tBkBootSec: %d\n", (int)bs->BkBootSec);
        printf("\tBS_DriveNumber: %d\n", (int)bs->BS_DriveNumber);
        printf("\tReserved:");
        for(int i=0;i>12;i++)
            printf(" %d", bs->Reserved[i]);
        printf("\n\tBS_Reserved1: %d\n", (int)bs->BS_Reserved1);
        printf("\tBS_BootSig: %d\n", (int)bs->BS_BootSig);
        printf("\tBS_VolumeID: %d\n", (int)bs->BS_VolumeID);
        printf("\tBS_VolumeLabel: %s\n", bs->BS_VolumeLabel);
        printf("\tBS_FileSystemType: %s\n", bs->BS_FileSystemType);
        cout << "\t*******************************************************" << endl;
    }

    void print_bpb(BPB_struct* bs){
        cout << "*******************************************************" << endl;
        printf("BS_JumpBoot: %s\n", bs->BS_JumpBoot);
        printf("BS_OEMName: %s\n", bs->BS_OEMName);
        printf("BytesPerSector: %d\n", (int)bs->BytesPerSector);
        printf("SectorsPerCluster: %d\n", (int)bs->SectorsPerCluster);
        printf("ReservedSectorCount: %d\n", (int)bs->ReservedSectorCount);
        printf("NumFATs: %d\n", (int)bs->NumFATs);
        printf("RootEntryCount: %d\n", (int)bs->RootEntryCount);
        printf("TotalSectors16: %d\n", (int)bs->TotalSectors16);
        printf("Media: 0x%x\n", (int)bs->Media);
        printf("FATSize16: %d\n", (int)bs->FATSize16);
        printf("SectorsPerTrack: %d\n", (int)bs->SectorsPerTrack);
        printf("NumberOfHeads: %d\n", (int)bs->NumberOfHeads);
        printf("HiddenSectors: %d\n", (int)bs->HiddenSectors);
        printf("TotalSectors32: %d\n", (int)bs->TotalSectors32);
        cout << "extended: " << endl;
        print_bpb32(&bs->extended);
        cout << "*******************************************************" << endl; 
    }

    void print_struct_FatFileLFN(struct_FatFileLFN* ff){
        printf("SequenceNumber: %01x\n", ff->sequence_number);
        printf("Filename: ");
        for(int i=0;i<5;i++) printf("%c", (wchar_t*)ff->name1[i]);
        for(int i=0;i<6;i++) printf("%c", (wchar_t*)ff->name2[i]);
        for(int i=0;i<2;i++) printf("%c", (wchar_t*)ff->name3[i]);
            cout << endl;
        
        printf("Attributes: %01x\n", ff->attributes);
        printf("Reserved: %01x\n", ff->reserved);
        printf("Checksum: %01x\n", ff->checksum);
        printf("FirstCluster: %02x\n", ff->firstCluster);
    }

    void print_struct_FatFile83(struct_FatFile83* ff){
        printf("Filename: ");
        for(int i=0;i<8;i++) printf("%lc", ff->filename[i]);
        cout << endl;
        printf("Extension: ");
        for(int i=0;i<3;i++) printf("%lc", ff->extension[i]);
        cout << endl;
        //printf("Extension: %s\n", (char*)ff->extension);
        printf("Attributes: %01x\n", ff->attributes);
        printf("Reserved: %01x\n", ff->reserved);
        printf("CreationTimeMs: %01x\n", ff->creationTimeMs);
        printf("CreationTime: %02x\n", ff->creationTime);
        printf("CreationDate: %02x\n", ff->creationDate);
        printf("LastAccessTime: %02x\n", ff->lastAccessTime);
        printf("EaIndex: %02x\n", ff->eaIndex);
        printf("ModifiedTime: %02x\n", ff->modifiedTime);
        printf("ModifiedDate: %02x\n", ff->modifiedDate);
        printf("FirstCluster: %02x\n", ff->firstCluster);
        printf("FileSize: %04x\n", ff->fileSize);
    }

    void print_FAT(){
        for(unsigned int i=0;i<bpb32.FATSize*BPS/4;i++){
            if(i>1 && !(i%16)) cout << endl;
            printf("%08x ", FAT[i]);
        }
        cout << endl;
    }

    void print_cluster(uint8_t* cluster, int noc){
        for(int i=0;i<bpb.SectorsPerCluster*BPS*noc;i++){
            if(i>1 && !(i%32)) cout << endl;
            printf("%02x ", cluster[i]);
        }
        cout << endl;
    }

    void print_entries(vector<sentry>* current_entries){
        for(int i=0;i<current_entries->size();i++){
            printf("Filename: %s\n", (*current_entries)[i].name);
            printf("Is Directory: %d\n", (*current_entries)[i].is_dir);
            printf("CreationTimeMs: %d\n", (*current_entries)[i].createTimeMs);
            printf("CreationTime: %d:%d:%d\n", (*current_entries)[i].creationTime[0],(*current_entries)[i].creationTime[1],(*current_entries)[i].creationTime[2]);
            printf("CreationDate: %d:%d:%d\n", (*current_entries)[i].creationDate[0],(*current_entries)[i].creationDate[1],(*current_entries)[i].creationDate[2]);
            printf("LastAccessTime: %d:%d:%d\n", (*current_entries)[i].lastAccessTime[0],(*current_entries)[i].lastAccessTime[1],(*current_entries)[i].lastAccessTime[2]);
            printf("ModifiedTime: %d:%d:%d\n", (*current_entries)[i].modifiedTime[0],(*current_entries)[i].modifiedTime[1],(*current_entries)[i].modifiedTime[2]);
            printf("ModifiedDate: %d:%d:%d\n", (*current_entries)[i].modifiedDate[0],(*current_entries)[i].modifiedDate[1],(*current_entries)[i].modifiedDate[2]);
            printf("eaIndex: %02x\n", (*current_entries)[i].eaIndex);
            printf("FirstCluster: %02x\n", (*current_entries)[i].firstCluster);
            printf("FileSize: %d\n", (*current_entries)[i].fileSize);
            cout << endl;
        }
    }

    void print_cluster_char(unsigned char* cluster){
        for(unsigned int i=0;i<bpb.SectorsPerCluster*BPS;i++){
            if(i>1 && !(i%32)) cout << endl;
            printf("%c ", cluster[i]);
        }
        cout << endl;
    }

    /*-----------------------------------------------------------------------*/
#endif