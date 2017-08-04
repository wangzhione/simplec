#include <mpack.h>

static void _do_something_with_data(char * data, size_t size) {
	// parse a file into a node tree
	mpack_tree_t tree;
	mpack_tree_init(&tree, data, size);
	mpack_node_t root = mpack_tree_root(&tree);

	// extract the example data on the msgpack homepage
	bool compact = mpack_node_bool(mpack_node_map_cstr(root, "compact"));
	int schema = mpack_node_i32(mpack_node_map_cstr(root, "schema"));

	// clean up and check for errors
	if (mpack_tree_destroy(&tree) != mpack_ok) {
		fprintf(stderr, "An error occurred decoding the data!\n");
		return;
	}

	fprintf(stdout, "{\"compact\":%s, \"schema\":%d}\n", compact ? "true" : "false", schema);
}

void test_mpack(void) {
		// encode to memory buffer
	char * data;
	size_t size;
	mpack_writer_t writer;
	mpack_writer_init_growable(&writer, &data, &size);

	// write the example on the msgpack homepage
	mpack_start_map(&writer, 2);
	mpack_write_cstr(&writer, "compact");
	mpack_write_bool(&writer, true);
	mpack_write_cstr(&writer, "schema");
	mpack_write_uint(&writer, 0);
	mpack_finish_map(&writer);

	// finish writing
	if (mpack_writer_destroy(&writer) != mpack_ok) {
		fprintf(stderr, "An error occurred encoding the data!\n");
		return;
	}

	// use the data
	_do_something_with_data(data, size);
	free(data);
}