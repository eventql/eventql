#include <ext/hash_map>
int main()
{
    const __gnu_cxx::hash_map<int,int> t;
    return t.find(5) == t.end();
}
