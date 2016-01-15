static type_info_t TYPES[] = { 
{ 0, 'l', sizeof(VP), "list", &repr_l },
{ 1, 't', sizeof(int), "tag", &repr_t },
{ 2, 'b', sizeof(int8_t), "byte", &repr_b },
{ 3, 'i', sizeof(int), "int", &repr_i },
{ 4, 'j', sizeof(__int64_t), "long", &repr_j },
{ 5, 'o', sizeof(__int128_t), "octo", &repr_o },
{ 6, 'c', sizeof(char), "char", &repr_c },
{ 7, 'd', sizeof(VP), "dict", &repr_d },
{ 8, '1', sizeof(unaryFunc*), "f1", &repr_1 },
{ 9, '2', sizeof(binaryFunc*), "f2", &repr_2 },
{ 10, 'p', sizeof(Proj), "proj", &repr_p },
{ 11, 'x', sizeof(VP), "ctx", &repr_x },
};
