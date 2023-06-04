#ifndef MISC_FUNC_H
#define MISC_FUNC_H

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "download_func.h"

using namespace std;

string get_new_file_name(std::string fileName);
void merge_vector(vector<down_stat>& v, int l, int m, int r, int (*comparator)(const down_stat*, const down_stat*));
void merge_sort(vector<down_stat>& v, int l, int r, int (*comparator)(const down_stat*, const down_stat*));

#endif /* MISC_FUNC_H */
