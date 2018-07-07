///------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Identifiers.hpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// This file contains the IdentifierTable and IdentifierInfo classes.
//
// Note: This class, and the whole idiom of storing the IdentifierInfo* instead of the raw string
// is more or less a premature optimization. (I got the idea from CLang, found it was pretty nice and implemented it
// without thinking if it was really needed.) I did not realize that at the time of doing it,
// but now it's done so It's going to stay that way ! Currently the IdentifierInfo 
// doesn't store anything other than an iterator to the string, but in the future it might 
// contain much more as the language grows.
//
////------------------------------------------------------////

#pragma once 

#include <map>
#include <string>

namespace fox
{
	class IdentifierInfo;

	// Wrapper around a const_iterator of a map entry, used to safely access the .first element (the string)
	class StringPtrInMap
	{
		public:
			typedef std::map<std::string, IdentifierInfo>::const_iterator ItTy;

			StringPtrInMap(ItTy iter);

			const std::string& get() const;

			ItTy it_;
	};

	// A Class holding informations related to a identifier.
	// Currently, it only stores the iterator to the std::string.
	class IdentifierInfo
	{
		public:
			IdentifierInfo(const StringPtrInMap::ItTy& iter);

			IdentifierInfo(IdentifierInfo&&) = default;
			IdentifierInfo(IdentifierInfo&) = delete;
			IdentifierInfo& operator=(IdentifierInfo&) = delete;

			// Returns the string naming this identifier
			const std::string& getStr() const;

			// Comparison operators for use with STL containers.
			bool operator<(const IdentifierInfo& id) const;
			bool operator<(const std::string& idstr) const;

			// Other comparison operators
			bool operator==(const IdentifierInfo& id) const;
			bool operator==(const std::string& str) const;

			bool operator!=(const IdentifierInfo& id) const;
			bool operator!=(const std::string& str) const;
	
		private:
			friend class IdentifierTable;

			StringPtrInMap mapIter_;
	};

	// A class that maps strings to IdentifierInfo.
	// This contains every (user-defined) identifier currently in use, and is populated by the 
	// Lexer.
	class IdentifierTable
	{
		private:
			using IDTableType = std::map<std::string, IdentifierInfo>;
			using IDTableIteratorType = IDTableType::iterator;
			using IDTableConstIteratorType = IDTableType::const_iterator;

		public:
			IdentifierTable() = default;

			// Returns the identifierinfo of the string "id" if it exists. 
			// If it does not exists, it creates a new entry into the table and returns it.
			IdentifierInfo* getUniqueIdentifierInfo(const std::string& id);
			IdentifierInfo* getInvalidID();

			// Returns true if the identifier exists in the map, false otherwise.
			bool exists(const std::string &id) const;

			// Iterators
			IDTableConstIteratorType begin() const;
			IDTableIteratorType begin();

			IDTableConstIteratorType end() const;
			IDTableIteratorType end();
		private:
			IdentifierInfo* invalidID_ = nullptr;

			// Deleted methods
			IdentifierTable(const IdentifierTable&) = delete;
			IdentifierTable& operator=(const IdentifierTable&) = delete;

			// Member variables
			IDTableType table_;
	};
}