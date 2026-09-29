#include "DB.hpp" 
#include <optional>
 

/* If key exists or field exists, overwrites 
 */

constexpr int value_type_index = 0; 

bool DB::set(KeyType key, FieldType field, ValueType value){
	auto &RecordMap = this->map[key];
	RecordMap.insert_or_assign(field, RecordType{std::move(value), {}, {}}); 
	return true;
}
/* If key or field of key doesn't exist, return std nullopt 
 */
auto DB::get(KeyType key, FieldType field) const -> std::optional<ValueType>{
	auto rec = map.find(key); 
	if(rec == map.end()) return std::nullopt; 
	const auto& RecordMap = rec->second; 
	auto it = RecordMap.find(field); 
	if(it == RecordMap.end()) return std::nullopt; 
	return it->second.value; 
}
/* If key or field doesn't exist, return false 
 */
bool DB::del(KeyType key, FieldType field){
	auto rec = map.find(key);
	if(rec == map.end()) return false; 
	auto& RecordMap = rec->second; 
	auto it = RecordMap.find(field); 
	if(it == RecordMap.end()) return false; 
	RecordMap.erase(it); 
	return true;
}

/* If key doesn't exist, returns std::nullopt 
 */
auto DB::scan(KeyType key) const -> std::optional<ScanType>{
	auto rec = map.find(key); 
	if(rec == map.end()){
		return std::nullopt; 
	}
	const auto& RecordMap = rec->second; 
	ScanType ret(RecordMap.size()); 
	for(const auto& [field, value]: RecordMap){
		ret.push_back(std::pair{field, value}); 
	}
	return ret; 
}

//start at key, used for paginated scans - you can always pass in the same prefix twice to get a regular prefix scan... 
auto DB::scan_by_prefix(KeyType key, KeyType prefix) const -> std::optional<ScanType>{
	auto rec = map.find(key); 
	if(rec == map.end()) return std::nullopt; 
	const auto& RecordMap = rec->second; 
	ScanType ret{}; 
	//start at whichever is later between key and prefix
	auto it = RecordMap.lower_bound(std::max(key, prefix));
	for(; it != RecordMap.end(); ++it){
		const auto& [k, v] = *it; 
		if(k.compare(0, prefix.size(), prefix) != 0) break; //left the prefix range 
		ret.emplace_back(k, v);
	}
	if(ret.empty()) return std::nullopt; 
	return ret;
}

bool DB::set_at(KeyType key, FieldType field, ValueType value, TimeType ts){
	auto &RecordMap = this->map[key]; 
	RecordMap.insert_or_assign(field, RecordType{std::move(value), ts, {}});
	return true;
}

auto DB::get_at(KeyType key, FieldType field, TimeType ts) -> std::optional<ValueType> {
	auto rec = map.find(key); 
	if(rec == map.end()) return std::nullopt; 
	auto& RecordMap = rec->second; 
	auto it = RecordMap.find(field); 
	if(it == RecordMap.end()) return std::nullopt; 
	if(it->second.expires_at <= ts){
		RecordMap.erase(it); 
		return std::nullopt; 
	}
	return it->second.value;
}

bool DB::del_at(KeyType key, FieldType field, TimeType ts){
	//b/c timestamps are monotonically increasing, we actually ignore ts for now... 
		//once this guarantee is gone / we need logs... change 
	auto rec = map.find(key);
	if(rec == map.end()) return false; 
	auto& RecordMap = rec->second; 
	auto it = RecordMap.find(field); 
	if(it == RecordMap.end()) return false; 
	if(it->second.created >= ts){
		//we can create and delete somethiing in the same ts 
#ifdef DEBUG 
		std::cout << "Tried to delete (" << key << ", " << field << " at timestep " << ts << " but was created at timestep " << it->second.created << '\n'; 
#endif
		return false; 
	}
	RecordMap.erase(it);
	return true;
}
bool DB::set_with_ttl(KeyType key, FieldType field, ValueType value, TimeType ts, TimeType TTL){
	auto &RecordMap = this->map[key]; 
	RecordMap.insert_or_assign(field, RecordType{std::move(value), ts, ts+TTL});
	return true;

}
auto DB::scan_at(KeyType key, TimeType ts) const -> std::optional<ScanType> {
	//bc timestamps are monotonically increasing, we can actually ignore ts for now... 
		//prepare for it to be gone 
	auto rec = map.find(key); 
	if(rec == map.end()){
		return std::nullopt; 
	}
	const auto& RecordMap = rec->second; 
	ScanType ret(RecordMap.size()); 
	for(const auto& [field, record]: RecordMap){
		if(record.expires_at <= ts) continue; //already expired 
		if(record.created > ts) continue; //does not exist yet 
		ret.push_back(std::pair{field, record}); 
	}
	return ret; 
}
auto DB::scan_by_prefix_at(KeyType key, KeyType prefix, TimeType ts) const -> std::optional<ScanType> {
	auto rec = map.find(key); 
	if(rec == map.end()) return std::nullopt; 
	const auto& RecordMap = rec->second; 
	ScanType ret{}; 
	//start at whichever is later between key and prefix
	auto it = RecordMap.lower_bound(std::max(key, prefix));
	for(; it != RecordMap.end(); ++it){
		const auto& [k, v] = *it; 
		if(k.compare(0, prefix.size(), prefix) != 0) break; //left the prefix range 
		if(v.expires_at <= ts) continue; //already expired 
		if(v.created > ts) continue; //does not exist yet 
		ret.emplace_back(k, v);
	}
	if(ret.empty()) return std::nullopt; 
	return ret;
}


