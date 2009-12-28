#pragma once

#include <map>
#include <rsiface/rstypes.h>

// ftChunkMap: 
// 	- handles chunk map over a complete file
// 	- mark down which chunk is being downloaded by which peer
// 	- allocate data ranges of any requested size for a given peer
// 		- continuing an existing chunk
// 		- allocating a new chunk
//
// Download mecanism:
// 	- ftFileCreator handles a list of active slices, and periodically resends requests every 20 sec.
// 		Slices have arbitrary size (less than a chunk), depending on the transfer rate.
// 		When receiving data, ftFileCreator shrinks its slices until they get complete. When a slice is finished, it
// 		notifies ftChunkMap that this slice is done.
//
// 	- ftChunkMap maintains two levels:
// 		- the chunk level (Chunks a 1MB long) with a map of who has which chunk and what locally is the state of 
// 		each chunk
// 		- the slice level: each active chunk is cut into slices (basically a list of intervalls) being downloaded, and
// 		a remaining slice to cut off new candidates. When notified for a complete slice, ftChunkMap removed the
// 		corresponding acive slice. When asked a slice, ftChunkMap cuts out a slice from the remaining part of the chunk
// 		to download, sends the slice's coordinates and gives a unique slice id (such as the slice offset).


// This class handles a slice of a chunk of arbitrary uint32_t size, at the level of ftFileCreator

class ftChunk 
{
	public:
		typedef uint64_t ChunkId ;

		ftChunk():offset(0), size(0), ts(0) {}

		friend std::ostream& operator<<(std::ostream& o,const ftChunk& f) ;

		uint64_t offset;
		uint64_t size;
		ChunkId  id ;
		time_t   ts;
};

// This class handles a single fixed-sized chunk. Although each chunk is requested at once,
// it may be sent back into sub-chunks because of file transfer rate constraints. 
// So the dataReceived function should be called to progressively complete the chunk,
// and the getChunk method should ask for a sub-chunk of a given size.
//
class Chunk
{
	public: 
		Chunk(): _start(0),_offset(0),_end(0) {}	// only used in default std::map fillers

		Chunk(uint64_t start,uint32_t size) ;

		void getSlice(uint32_t size_hint,ftChunk& chunk) ;

		// Returns true when the chunk is complete
		bool empty() const { return _offset == _end ; }

		// Array of intervalls of bytes to download.
		//
		uint64_t _start ;		// const
		uint64_t _offset ;	// not const: handles the current offset within the chunk.
		uint64_t _end ;		// const
};

class ChunkDownloadInfo
{
	public:
		std::map<ftChunk::ChunkId,uint32_t> _slices ;
		uint32_t _remains ;
};

class ChunkMap
{
   public:
		typedef uint32_t ChunkNumber ;

		// Constructor. Decides what will be the size of chunks and how many there will be.

		ChunkMap(uint64_t file_size) ;

		// constructor from saved map info
		ChunkMap(uint64_t file_size,const std::vector<uint32_t>& map,uint32_t chunk_size,uint32_t chunk_number,FileChunksInfo::ChunkStrategy s) ;

		// destructor
		virtual ~ChunkMap() {}

      // Returns an slice of data to be asked to the peer within a chunk.
		// If a chunk is already been downloaded by this peer, take a slice at
		// the beginning of this chunk, or at least where it starts.
		// If not, randomly/streamly select a new chunk depending on the strategy. 
      // adds an entry in the chunk_ids map, and sets up 1 interval for it.
      // the chunk should be available from the designated peer. 

      virtual bool getDataChunk(const std::string& peer_id,uint32_t size_hint,ftChunk& chunk) ; 

      // Notify received a slice of data. This needs to 
      //   - carve in the map of chunks what is received, what is not.
      //   - tell which chunks are finished. For this, each interval must know what chunk number it has been attributed
      //    when the interval is split in the middle, the number of intervals for the chunk is increased. If the interval is
      //    completely covered by the data, the interval number is decreased.

      virtual void dataReceived(const ftChunk::ChunkId& c_id) ;

      // Decides how chunks are selected. 
      //    STREAMING: the 1st chunk is always returned
      //       RANDOM: the beginning of a random interval is selected first. If two few intervals 
      //                exist, the largest one is randomly split into two.

		void setStrategy(FileChunksInfo::ChunkStrategy s) { _strategy = s ; }

      // Properly fills an vector of fixed size chunks with availability or download state.
      // chunks is given with the proper number of chunks and we have to adapt to it. This can be used
      // to display square chunks in the gui or display a blue bar of availability by collapsing info from all peers.

      void buildAvailabilityMap(std::vector<uint32_t>& map,uint32_t& chunk_size,uint32_t& chunk_number,FileChunksInfo::ChunkStrategy& s) const ;
		void loadAvailabilityMap(const std::vector<uint32_t>& map,uint32_t chunk_size,uint32_t chunk_number,FileChunksInfo::ChunkStrategy s) ;

		// Updates the peer's availablility map
		//
		void setPeerAvailabilityMap(const std::string& peer_id,uint32_t chunk_size,uint32_t nb_chunks,const std::vector<uint32_t>& peer_map) ;

		// Returns the total size of downloaded data in the file.
		uint64_t getTotalReceived() const { return _total_downloaded ; }

		void getChunksInfo(FileChunksInfo& info) const ;
	protected:
		// handles what size the last chunk has.
		uint32_t sizeOfChunk(uint32_t chunk_number) const ;

		// Returns the first chunk available starting from start_location for this peer_id.
		//
		uint32_t getAvailableChunk(uint32_t start_location,const std::string& peer_id) ;

	private:
		uint64_t												_file_size ;		// total size of the file in bytes.
		uint32_t												_chunk_size ;		// Size of chunks. Common to all chunks.
		FileChunksInfo::ChunkStrategy 				_strategy ;			// how do we allocate new chunks
		std::map<std::string,Chunk>					_active_chunks_feed ; // vector of chunks being downloaded. Exactly one chunk per peer id.
		std::map<ChunkNumber,ChunkDownloadInfo>	_slices_to_download ; // list of (slice id,slice size) 

		std::vector<FileChunksInfo::ChunkState>	_map ;				// vector of chunk state over the whole file

		std::map<std::string,std::vector<uint32_t> >	_peers_chunks_availability ;	// what does each source peer have, stored in compressed format.

		uint64_t												_total_downloaded ;
};


