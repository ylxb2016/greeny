#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../src/err.h"
#include "../src/vector.h"
#include "../src/libannouncebulk.h"

#define ASSERT_OK() assert_int_equal( in_err, GRN_OK );

static void test_sanity( void **state ) {
	( void ) state;
}

static void test_vector( void **state ) {
	( void ) state;
	int in_err;

	struct vector *vec = vector_alloc( &in_err );
	ASSERT_OK();
	assert_int_equal( vec->allocated_n, 1 );
	assert_int_equal( vec->used_n, 0 );
	assert_non_null( vec->buffer );

	int a = 42, b = 43;
	vector_push( vec, &a, &in_err );
	ASSERT_OK();
	vector_push( vec, &b, &in_err );
	ASSERT_OK();

	assert_int_equal( vec->allocated_n, 2 );
	assert_int_equal( vec->used_n, 2 );
	assert_int_equal( *( ( int * )vec->buffer[1] ), b );

	int export_length;
	void **export_buffer = vector_export( vec, &export_length );
	assert_int_equal( export_length, 2 );
	assert_ptr_equal( export_buffer, vec->buffer );

	vector_free( vec );
}

char *strsubst( const char *haystack, const char *find, const char *replace, int *out_err );
static void test_strsubst( void **state ) {
	( void ) state;
	int in_err;

	// no substitution
	char *hello_world = strsubst( "hello world", "the cat is in the bag", "roger", &in_err );
	ASSERT_OK();
	assert_string_equal( hello_world, "hello world" );
	free( hello_world );

	// substitution in the middle
	char *goodbye_sweet_world = strsubst( "goodbye ugly world", "ugly", "sweet", &in_err );
	ASSERT_OK();
	assert_string_equal( goodbye_sweet_world, "goodbye sweet world" );
	free( goodbye_sweet_world );

	// substitution at the beginning
	char *eat_world = strsubst( "hello world", "hello", "eat", &in_err );
	ASSERT_OK();
	assert_string_equal( eat_world, "eat world" );
	free( eat_world );

	// substitution at the end
	char *hello_mom = strsubst( "hello world", "world", "mom", &in_err );
	ASSERT_OK();
	assert_string_equal( hello_mom, "hello mom" );
	free( hello_mom );

	// multiple needles (should only replace first)
	// originally i accidentarlly replaced hello instead of world then it said "mom world"
	// and that sounds super fucking funny
	char *hello_mom_hello_mom = strsubst( "hello world. hello...mom?", "hello", "mom", &in_err );
	ASSERT_OK();
	assert_string_equal( hello_mom_hello_mom, "mom world. hello...mom?" );
	free( hello_mom_hello_mom );

	// Output is drastically longer than input
	char *hello_globglogabgolab_mom = strsubst( "hello my mom", "my", "globglogabgolab", &in_err );
	ASSERT_OK();
	assert_string_equal( hello_globglogabgolab_mom, "hello globglogabgolab mom" );
	free( hello_globglogabgolab_mom );
}

void transform_buffer( struct grn_ctx *ctx, int *out_err );

// line numbers aren't reported right when it's a real function. kekek
void _assert_transform_buffer_single( const char *buffer, struct grn_transform transform, char *expected_buffer ) {
	int in_err;

	struct grn_ctx my_ctx = {
		.buffer = malloc( 256 ),
		.buffer_n = strlen( buffer ), // they don't need to know about that silly null byte
		.transforms = &transform,
		.transforms_n = 1,
	};
	strcpy( my_ctx.buffer, buffer );
	transform_buffer( &my_ctx, &in_err );
	ASSERT_OK();
	assert_memory_equal( my_ctx.buffer, expected_buffer, my_ctx.buffer_n );
	free( my_ctx.buffer );
}

// test buffer transforms when they will do the transform as expected.
static void test_transform_buffer( void **state ) {
	( void ) state;
	int in_err;

	char *key_dummy[1];
	key_dummy[0] = NULL;
	char *key_presto[2];
	key_presto[0] = "presto";
	key_presto[1] = NULL;
	struct grn_transform transform_set_presto = GRN_MKTRANSFORM_SET_STRING( "presto", "largo" );
	transform_set_presto.key = key_dummy;
	struct grn_transform transform_del_presto = GRN_MKTRANSFORM_DELETE( "presto" );
	transform_del_presto.key = key_dummy;
	struct grn_transform transform_sub_presto = GRN_MKTRANSFORM_SUBSTITUTE( "rgo", "pd" );
	transform_sub_presto.key = key_dummy;

	struct grn_transform transform_del_inner_presto = transform_del_presto;
	transform_del_inner_presto.key = key_presto;

	// test when they should work normally
	_assert_transform_buffer_single( "de", transform_set_presto, "d6:presto5:largoe" );
	_assert_transform_buffer_single( "d6:presto5:largoe", transform_del_presto, "de" );
	_assert_transform_buffer_single( "5:largo", transform_sub_presto, "4:lapd" );
	_assert_transform_buffer_single( "l5:largo10:overgottene", transform_sub_presto, "l4:lapd9:ovepdttene" );

	// test no-ops
	_assert_transform_buffer_single( "de", transform_del_presto, "de" );
	_assert_transform_buffer_single( "d6:presto5:largoe", transform_set_presto, "d6:presto5:largoe" );
	_assert_transform_buffer_single( "d6:presto4:lapde", transform_sub_presto, "d6:presto4:lapde" );

	// test incorrect types
	_assert_transform_buffer_single( "6:presto", transform_set_presto, "6:presto" );
	_assert_transform_buffer_single( "6:presto", transform_del_presto, "6:presto" );
	_assert_transform_buffer_single( "6:presto", transform_sub_presto, "6:presto" );

	// test invalid key
	// the substitute transform is already setup, but substitute has some special stuff to test data types so that it works with arrays. So, it is better to use del here.
	_assert_transform_buffer_single( "de", transform_del_inner_presto, "de" );

	// TODO: test deep keys
	// TODO: test deep substitute in lists
}

int main( void ) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test( test_sanity ),
		cmocka_unit_test( test_vector ),
		cmocka_unit_test( test_strsubst ),
		cmocka_unit_test( test_transform_buffer ),
	};

	return cmocka_run_group_tests( tests, NULL, NULL );
}