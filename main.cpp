
#include "utilities.h"
#include <cstring>

vector<string> current_dir_name;
string entry;
vector<string> command;

int main(int argc, char const *argv[])
{
    init(argv[1]);
    current_dir_name.push_back("/");

    while(1){
        cout << retrieve_path(current_dir_name) << "> ";
        getline(cin, entry);
        parse_input(&entry, &command);

        uint32_t tmp_cluster_number;

        if(command[0] == "quit") break;

        else if(command[0] == "cd"){
            if(command.size()>1){
                if(((int)(tmp_cluster_number = find_path(command[1]).first) > -1) && find_path(command[1]).second){
                    current_dir_number = tmp_cluster_number;
                    current_cluster_chain = get_chain(current_dir_number);
                    read_cluster(current_dir_number, current_cluster_head);
                    vector<string> pv = find_path_vector(command[1]);
                    if(pv[0] == "/"){
                        current_dir_name.clear();
                        current_dir_name.push_back("/");
                    }
                    
                    for(int l=0;l<pv.size();l++){
                        if(pv[l] == ".") continue;
                        else if(pv[l] == ".."){
                            if(current_dir_name.size() != 1) current_dir_name.pop_back();
                            current_dir_name.pop_back();
                        }
                        else{
                            if((current_dir_name.size() > 1 && current_dir_name[0] == "/")) current_dir_name.push_back("/");
                            if((pv[l] != "/")) current_dir_name.push_back(pv[l]);
                        }
                    }
                    
                }
            }
            else{
                current_dir_number = bpb32.RootCluster;
                current_cluster_chain = get_chain(current_dir_number);
                read_cluster(current_dir_number, current_cluster_head);
                current_dir_name.clear(); current_dir_name.push_back("/");
            }
        }
        else if(command[0] == "ls"){
            int pad = 0;
            if(command.size()>2){ // <path> > ls -l <path2>
                if(((int)(tmp_cluster_number = find_path(command[2]).first) > -1) && find_path(command[2]).second){
                    vector<sentry> res = list_entry_props(tmp_cluster_number);
                    for(int i=0;i<res.size();i++){
                        if(res[i].is_dir) cout << "drwx------ 1 root root 0 " 
                                               << month[res[i].modifiedDate[1]-1] << " " << res[i].modifiedDate[2] << " " << res[i].modifiedTime[0] << ":" << res[i].modifiedTime[1] << " "
                                               << res[i].name << endl;
                        else cout << "-rwx------ 1 root root " << res[i].fileSize << " "
                                  << month[res[i].modifiedDate[1]-1] << " " << res[i].modifiedDate[2] << " " << res[i].modifiedTime[0] << ":" << res[i].modifiedTime[1] << " "
                                  << res[i].name << endl;
                    }
                }
            }
            else if(command.size()==2){
                if(command[1] == "-l"){ // <path> > ls -l
                    vector<sentry> res = list_entry_props(current_dir_number);
                    for(int i=0;i<res.size();i++){
                        if(res[i].is_dir) cout << "drwx------ 1 root root 0 " 
                                               << month[res[i].modifiedDate[1]-1] << " " << res[i].modifiedDate[2] << " " << res[i].modifiedTime[0] << ":" << res[i].modifiedTime[1] << " "
                                               << res[i].name << endl;
                        else cout << "-rwx------ 1 root root " << res[i].fileSize << " "
                                  << month[res[i].modifiedDate[1]-1] << " " << res[i].modifiedDate[2] << " " << res[i].modifiedTime[0] << ":" << res[i].modifiedTime[1] << " "
                                  << res[i].name << endl;
                    }

                }
                else{ // <path> > ls <path2>
                    if(((int)(tmp_cluster_number = find_path(command[1]).first) > -1) && find_path(command[1]).second){
                        vector<pair<string,pair<uint32_t,bool>>> res = list_entry_names(tmp_cluster_number);
                        for(int i=0;i<res.size();i++){
                            if(pad>0 && !(pad%20)) cout << endl;
                            if(res[i].first != "."  && res[i].first != ".."){ cout << res[i].first << " "; pad++;}
                        }
                        if(res.size())cout << endl;
                    }

                }
            }
            else{ // <path> > ls
                vector<pair<string,pair<uint32_t,bool>>> res = list_entry_names(current_dir_number);
                for(int i=0;i<res.size();i++){
                    if(pad>0 && !(pad%20)) cout << endl;
                    if(res[i].first != "."  && res[i].first != ".."){ cout << res[i].first << " "; pad++;}
                }
                if(res.size())cout << endl;

            }
        }
        else if(command[0] == "mkdir"){
            vector<string> actual_path = find_path_vector(command[1]);
            string new_name = actual_path[actual_path.size()-1];
            string parsed_path = "";
            for(int i=0;i<actual_path.size()-1;i++){
                parsed_path += actual_path[i];
            }
            if((((int)(tmp_cluster_number = find_path(parsed_path).first) > -1) && find_path(parsed_path).second) || (int)(find_path(command[1]).first == -1)){

                int data_index;
                int fat_index;
                vector<struct_FatFileEntry> test;
                uint32_t target_cluster;
                string new_name = find_path_vector(command[1])[find_path_vector(command[1]).size()-1];
                int needed_free_entry = new_name.size()/11+2;
                bool trancuate=false;

                if((int)(tmp_cluster_number) == -1) data_index = find_empty_data_entry(current_cluster_chain);
                else data_index = find_empty_data_entry(get_chain(tmp_cluster_number));
                if(data_index == -1){
                    data_index = 0;
                    int dir_FAT = find_empty_FAT_entry();
                    FAT[dir_FAT] = (uint32_t) 0x0ffffff8;
                    if((int)(tmp_cluster_number) == -1){
                        fat_index = current_cluster_chain[current_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                    }
                    else{
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        fat_index = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                    }
                }

                if((bpb.SectorsPerCluster*BPS/32)-data_index < needed_free_entry){
                    trancuate = true;
                    int dir_FAT = find_empty_FAT_entry();
                    FAT[dir_FAT] = (uint32_t) 0x0ffffff8;
                    if((int)(tmp_cluster_number) == -1){
                        fat_index = current_cluster_chain[current_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                        current_cluster_chain = get_chain(current_dir_number);
                    }
                    else{
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        fat_index = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                    }
                }
                int new_index = find_empty_FAT_entry();
                FAT[new_index] = (uint32_t) 0x0ffffff8;               

                if(!trancuate){
                    if((int)(tmp_cluster_number) == -1){ //current
                        target_cluster = current_cluster_chain[current_cluster_chain.size()-1];
                        test = generate_data_entry(new_name, 1, current_dir_number, new_index, get_tilda_index(current_dir_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    else{ //tmp
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        target_cluster = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        test = generate_data_entry(new_name, 1, tmp_cluster_number, new_index, get_tilda_index(tmp_cluster_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    struct_FatFileEntry aaaa[test.size()-1];
                    for(int i=0;i<test.size();i++) aaaa[i] = aaaa[i] = test[test.size()-1-i];
                    for(int i=0;i<test.size()-1;i++){ ((FatFileEntry *)target2_cluster_head)[data_index+i] = test[i];}
                    ((FatFileEntry *)target2_cluster_head)[data_index+test.size()-1] = test[test.size()-1];
                    lseek(image, get_cluster_offset(target_cluster), SEEK_SET);
                    write(image, target2_cluster_head, bpb.SectorsPerCluster*BPS);
                }
                else{
                    if((int)(tmp_cluster_number) == -1){ //current
                        target_cluster = current_cluster_chain[current_cluster_chain.size()-2];
                        test = generate_data_entry(new_name, 1, current_dir_number, new_index, get_tilda_index(current_dir_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    else{ //tmp
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        target_cluster = tmp_cluster_chain[tmp_cluster_chain.size()-2];
                        test = generate_data_entry(new_name, 1, tmp_cluster_number, new_index, get_tilda_index(tmp_cluster_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }

                    struct_FatFileEntry aaaa[test.size()];
                    int contval = 0;
                    for(int i=0;i<test.size();i++){ aaaa[i] = test[test.size()-1-i]; /*printf("%02x\n", aaaa[i].lfn.sequence_number);*/}
                    for(int i=0;i<test.size();i++){
                        if(data_index+i < (bpb.SectorsPerCluster*BPS/32)){
                            ((FatFileEntry *)target2_cluster_head)[data_index+i] = test[i];
                        }
                        else{
                            contval = i;
                            break;
                        }
                    }
                    lseek(image, get_cluster_offset(target_cluster), SEEK_SET);
                    write(image, target2_cluster_head, bpb.SectorsPerCluster*BPS);

                    if((int)(tmp_cluster_number) == -1){ //current
                        target_cluster = current_cluster_chain[current_cluster_chain.size()-1];
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    else{ //tmp
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        target_cluster = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        read_cluster(target_cluster, target2_cluster_head);
                    }

                    for(int i=contval;i<test.size();i++){ 
                        ((FatFileEntry *)target2_cluster_head)[i-contval] = test[i];
                    }

                    lseek(image, get_cluster_offset(target_cluster), SEEK_SET);
                    write(image, target2_cluster_head, bpb.SectorsPerCluster*BPS);
                    
                }
            }
        }
        else if(command[0] == "touch"){
            vector<string> actual_path = find_path_vector(command[1]);
            string new_name = actual_path[actual_path.size()-1];
            string parsed_path = "";
            for(int i=0;i<actual_path.size()-1;i++){
                parsed_path += actual_path[i];
            }
            if((((int)(tmp_cluster_number = find_path(parsed_path).first) > -1) && find_path(parsed_path).second) || (int)(find_path(command[1]).first == -1)){

                int data_index;
                int fat_index;
                vector<struct_FatFileEntry> test;
                uint32_t target_cluster;
                string new_name = find_path_vector(command[1])[find_path_vector(command[1]).size()-1];
                int needed_free_entry = new_name.size()/11+2;
                bool trancuate=false;

                if((int)(tmp_cluster_number) == -1) data_index = find_empty_data_entry(current_cluster_chain);
                else data_index = find_empty_data_entry(get_chain(tmp_cluster_number));
                if(data_index == -1){
                    data_index = 0;
                    int dir_FAT = find_empty_FAT_entry();
                    FAT[dir_FAT] = (uint32_t) 0x0ffffff8;
                    if((int)(tmp_cluster_number) == -1){
                        fat_index = current_cluster_chain[current_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                    }
                    else{
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        fat_index = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                    }
                }

                if((bpb.SectorsPerCluster*BPS/32)-data_index < needed_free_entry){
                    trancuate = true;
                    int dir_FAT = find_empty_FAT_entry();
                    FAT[dir_FAT] = (uint32_t) 0x0ffffff8;
                    if((int)(tmp_cluster_number) == -1){
                        fat_index = current_cluster_chain[current_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                        current_cluster_chain = get_chain(current_dir_number);
                    }
                    else{
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        fat_index = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        FAT[fat_index] = dir_FAT;
                    }
                }               

                if(!trancuate){
                    if((int)(tmp_cluster_number) == -1){ //current
                        target_cluster = current_cluster_chain[current_cluster_chain.size()-1];
                        test = generate_data_entry(new_name, 0, current_dir_number, 0, get_tilda_index(current_dir_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    else{ //tmp
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        target_cluster = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        test = generate_data_entry(new_name, 0, tmp_cluster_number, 0, get_tilda_index(tmp_cluster_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }

                    struct_FatFileEntry aaaa[test.size()-1];
                    for(int i=0;i<test.size();i++) aaaa[i] = aaaa[i] = test[test.size()-1-i];
                    for(int i=0;i<test.size()-1;i++){ ((FatFileEntry *)target2_cluster_head)[data_index+i] = test[i];}
                    ((FatFileEntry *)target2_cluster_head)[data_index+test.size()-1] = test[test.size()-1];
                    lseek(image, get_cluster_offset(target_cluster), SEEK_SET);
                    write(image, target2_cluster_head, bpb.SectorsPerCluster*BPS);
                }
                else{
                    if((int)(tmp_cluster_number) == -1){ //current
                        target_cluster = current_cluster_chain[current_cluster_chain.size()-2];
                        test = generate_data_entry(new_name, 0, current_dir_number, 0, get_tilda_index(current_dir_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    else{ //tmp
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        target_cluster = tmp_cluster_chain[tmp_cluster_chain.size()-2];
                        test = generate_data_entry(new_name, 0, tmp_cluster_number, 0, get_tilda_index(tmp_cluster_number));
                        read_cluster(target_cluster, target2_cluster_head);
                    }

                    struct_FatFileEntry aaaa[test.size()];
                    int contval = 0;
                    for(int i=0;i<test.size();i++){ aaaa[i] = test[test.size()-1-i]; /*printf("%02x\n", aaaa[i].lfn.sequence_number);*/}
                    for(int i=0;i<test.size();i++){
                        if(data_index+i < (bpb.SectorsPerCluster*BPS/32)){
                            ((FatFileEntry *)target2_cluster_head)[data_index+i] = test[i];
                        }
                        else{
                            contval = i;
                            break;
                        }
                    }

                    lseek(image, get_cluster_offset(target_cluster), SEEK_SET);
                    write(image, target2_cluster_head, bpb.SectorsPerCluster*BPS);

                    if((int)(tmp_cluster_number) == -1){ //current
                        target_cluster = current_cluster_chain[current_cluster_chain.size()-1];
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    else{ //tmp
                        vector<uint32_t> tmp_cluster_chain = get_chain(tmp_cluster_number);
                        target_cluster = tmp_cluster_chain[tmp_cluster_chain.size()-1];
                        read_cluster(target_cluster, target2_cluster_head);
                    }
                    for(int i=contval;i<test.size();i++){
                        ((FatFileEntry *)target2_cluster_head)[i-contval] = test[i];
                    }

                    lseek(image, get_cluster_offset(target_cluster), SEEK_SET);
                    write(image, target2_cluster_head, bpb.SectorsPerCluster*BPS);

                }
            }

            
        }

        else if(command[0] == "cat"){
            if(command.size()>1){    
                if(((int)(tmp_cluster_number = find_path(command[1]).first) > -1) && !find_path(command[1]).second){
                    if(tmp_cluster_number != bpb32.RootCluster){
                        vector<uint32_t> res = get_chain(tmp_cluster_number);
                        for(int i=0;i<res.size();i++){
                            uint8_t* reading = new uint8_t[bpb.SectorsPerCluster*BPS];
                            read_cluster(res[i], reading);
                            for(int j=0;j<bpb.SectorsPerCluster*BPS;j++) printf("%c", reading[j]);
                            delete[] reading;
                        }
                        cout << endl;
                    }
                }
            }
        }
    }

    return 0;
}
