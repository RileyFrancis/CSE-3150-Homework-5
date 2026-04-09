import subprocess
import os
import pytest
import tempfile
import shutil


def setup_module(module):
    """Compile the solution using Makefile before running tests"""
    # First, clean any previous builds
    subprocess.run(["make", "clean"], capture_output=True)

    # Then compile the solution
    result = subprocess.run(["make"], capture_output=True, text=True)
    if result.returncode != 0:
        pytest.fail(f"Failed to compile solution.cpp:\n{result.stderr}")

    # Verify the executable exists
    if not os.path.exists("./solution"):
        pytest.fail("Solution executable not found after compilation")


@pytest.fixture(autouse=True)
def clean_state():
    """Ensure clean state before and after each test"""
    # Clean up before test
    if os.path.exists("game_output.csv"):
        os.remove("game_output.csv")

    yield  # Run the test

    # Clean up after test
    if os.path.exists("game_output.csv"):
        os.remove("game_output.csv")


def write_input_board(board):
    """Write a board state to game_input.csv"""
    with open("game_input.csv", "w") as f:
        for row in board:
            f.write(",".join(map(str, row)) + "\n")


def run_cpp_game(inputs, initial_board=None):
    """Run the C++ game with given inputs and optional initial board"""
    if initial_board:
        write_input_board(initial_board)

    if os.path.exists("game_output.csv"):
        os.remove("game_output.csv")

    subprocess.run(["./solution"],
                  input=inputs.encode(),
                  capture_output=True,
                  timeout=5)

    results = []
    if os.path.exists("game_output.csv"):
        with open("game_output.csv") as f:
            for line in f:
                parts = line.strip().split(",")
                if len(parts) >= 17:
                    stage = parts[0]
                    board_data = list(map(int, parts[1:17]))
                    board = [board_data[i*4:(i+1)*4] for i in range(4)]
                    results.append((stage, board))
    return results


class TestMerges:
    """Test merge mechanics in all directions"""

    @pytest.mark.parametrize("initial,expected", [
        ([[2, 2, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[4, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[2, 2, 4, 4], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[4, 8, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[2, 2, 2, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[4, 4, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[4, 2, 2, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[4, 4, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[0, 2, 2, 4], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[4, 4, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[8, 8, 8, 8], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[16, 16, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
    ])
    def test_left_merge(self, initial, expected):
        """Test left movement merges"""
        results = run_cpp_game("a\nq\n", initial)

        merge_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
                break

        assert merge_board is not None, "Should have merge stage"
        assert merge_board == expected

    @pytest.mark.parametrize("initial,expected", [
        ([[0, 0, 2, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[0, 0, 0, 4], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[4, 4, 2, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[0, 0, 8, 4], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[2, 2, 2, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[0, 0, 4, 4], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
    ])
    def test_right_merge(self, initial, expected):
        """Test right movement merges"""
        results = run_cpp_game("d\nq\n", initial)

        merge_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
                break

        assert merge_board is not None, "Should have merge stage"
        assert merge_board == expected

    @pytest.mark.parametrize("initial,expected", [
        ([[2, 0, 0, 0], [2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[4, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[2, 4, 0, 0], [2, 4, 0, 0], [4, 2, 0, 0], [4, 2, 0, 0]],
         [[4, 8, 0, 0], [8, 4, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        ([[2, 0, 0, 0], [2, 0, 0, 0], [2, 0, 0, 0], [2, 0, 0, 0]],
         [[4, 0, 0, 0], [4, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
    ])
    def test_up_merge(self, initial, expected):
        """Test up movement merges"""
        results = run_cpp_game("w\nq\n", initial)

        merge_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
                break

        assert merge_board is not None, "Should have merge stage"
        assert merge_board == expected

    @pytest.mark.parametrize("initial,expected", [
        ([[2, 0, 0, 0], [2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]],
         [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [4, 0, 0, 0]]),

        ([[2, 4, 0, 0], [2, 4, 0, 0], [4, 2, 0, 0], [4, 2, 0, 0]],
         [[0, 0, 0, 0], [0, 0, 0, 0], [4, 8, 0, 0], [8, 4, 0, 0]]),

        ([[2, 0, 0, 0], [2, 0, 0, 0], [2, 0, 0, 0], [2, 0, 0, 0]],
         [[0, 0, 0, 0], [0, 0, 0, 0], [4, 0, 0, 0], [4, 0, 0, 0]]),
    ])
    def test_down_merge(self, initial, expected):
        """Test down movement merges"""
        results = run_cpp_game("s\nq\n", initial)

        merge_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
                break

        assert merge_board is not None, "Should have merge stage"
        assert merge_board == expected


class TestGameMechanics:
    """Test various game mechanics"""

    def test_no_double_merge(self):
        """Test that tiles don't merge twice in one move"""
        initial = [[2, 2, 2, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]
        results = run_cpp_game("a\nq\n", initial)

        merge_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
                break

        assert merge_board is not None, "Should have merge stage"
        assert merge_board[0] == [4, 4, 0, 0], f"Should not double merge"

    def test_spawn_after_valid_move(self):
        """Test that spawn happens after valid moves"""
        initial = [[2, 0, 0, 0], [0, 2, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]
        results = run_cpp_game("a\nq\n", initial)

        merge_board = None
        spawn_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
            elif stage == "spawn":
                spawn_board = board

        assert merge_board is not None, "Should have merge stage"
        assert spawn_board is not None, "Should have spawn stage"

        # Count tiles
        merge_count = sum(1 for r in range(4) for c in range(4) if merge_board[r][c] != 0)
        spawn_count = sum(1 for r in range(4) for c in range(4) if spawn_board[r][c] != 0)

        assert spawn_count == merge_count + 1, "Spawn should add exactly one tile"

        # Find spawned tiles and verify they are valid
        spawned_tiles = []
        for r in range(4):
            for c in range(4):
                if merge_board[r][c] == 0 and spawn_board[r][c] != 0:
                    spawned_tiles.append(spawn_board[r][c])

        assert len(spawned_tiles) == 1, f"Should spawn exactly one tile, found {len(spawned_tiles)}"
        assert spawned_tiles[0] in [2, 4], f"Spawned tile should be 2 or 4, got {spawned_tiles[0]}"

    def test_no_spawn_invalid_move(self):
        """Test that no spawn happens after invalid moves"""
        initial = [[2, 4, 8, 16], [32, 64, 128, 256], [512, 1024, 2048, 4096], [8192, 16384, 32768, 65536]]
        results = run_cpp_game("a\nq\n", initial)

        found_invalid = False
        for stage, board in results:
            if stage == "invalid":
                found_invalid = True
                assert board == initial

        assert found_invalid, "Should mark invalid moves"

    def test_undo(self):
        """Test undo functionality"""
        initial = [[2, 0, 0, 0], [0, 2, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]
        results = run_cpp_game("a\nu\nq\n", initial)

        found_undo = False
        for stage, board in results:
            if stage == "undo":
                found_undo = True
                assert board == initial

        assert found_undo, "Should have undo stage"

    def test_board_values(self):
        """Test that all board values are valid powers of 2"""
        initial = [[2, 2, 4, 4], [8, 8, 16, 16], [32, 32, 64, 64], [128, 128, 256, 256]]
        results = run_cpp_game("aawwddss\nq\n", initial)

        valid_values = {0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536}

        for stage, board in results:
            for row in board:
                for val in row:
                    assert val in valid_values, f"Invalid tile value {val}"


class TestSparseBoards:
    """Test moves on sparse boards"""

    @pytest.mark.parametrize("board_idx,initial,move,expected_stage,expected_board", [
        # Board with single tile at top-left
        (0, [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'a',
         "invalid", [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
        (0, [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'd',
         "merge", [[0, 0, 0, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
        (0, [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'w',
         "invalid", [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
        (0, [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 's',
         "merge", [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [2, 0, 0, 0]]),

        # Board with single tile at top-right
        (1, [[0, 0, 0, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'a',
         "merge", [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),
        (1, [[0, 0, 0, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'd',
         "invalid", [[0, 0, 0, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        # Board with single tile at bottom-left
        (2, [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [2, 0, 0, 0]], 'a',
         "invalid", [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [2, 0, 0, 0]]),
        (2, [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [2, 0, 0, 0]], 'w',
         "merge", [[2, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]]),

        # Board with single tile at bottom-right
        (3, [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 2]], 'a',
         "merge", [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [2, 0, 0, 0]]),
        (3, [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 2]], 'd',
         "invalid", [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 2]]),
    ])
    def test_sparse_moves(self, board_idx, initial, move, expected_stage, expected_board):
        """Test moves on boards with single tiles"""
        results = run_cpp_game(f"{move}\nq\n", initial)

        found_stage = False
        for stage, board in results:
            if stage in ["merge", "invalid"]:
                found_stage = True
                assert stage == expected_stage
                assert board == expected_board
                break

        assert found_stage, f"Should have merge or invalid stage"


class TestEdgeCases:
    """Test edge cases and special scenarios"""

    def test_full_board_no_moves(self):
        """Test that certain full boards can't move in some directions"""
        initial = [[2, 4, 2, 4], [4, 2, 4, 2], [2, 4, 2, 4], [4, 2, 4, 2]]

        # This board can't move left or up
        for move, direction in [('a', "left"), ('w', "up")]:
            results = run_cpp_game(f"{move}\nq\n", initial)
            found_invalid = False
            for stage, board in results:
                if stage == "invalid":
                    found_invalid = True
                    assert board == initial
                    break
            assert found_invalid, f"Should mark {direction} as invalid"

    def test_full_board_with_merges(self):
        """Test merges on a full board that can merge"""
        initial = [[2, 2, 4, 4], [8, 8, 16, 16], [32, 32, 64, 64], [128, 128, 256, 256]]
        results = run_cpp_game("a\nq\n", initial)

        found_merge = False
        for stage, board in results:
            if stage == "merge":
                found_merge = True
                assert board[0] == [4, 8, 0, 0]
                assert board[1] == [16, 32, 0, 0]
                assert board[2] == [64, 128, 0, 0]
                assert board[3] == [256, 512, 0, 0]
                break

        assert found_merge, "Should be able to merge"

    @pytest.mark.parametrize("initial,move,expected_first_row", [
        ([[2, 2, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'a', [4, 0, 0, 0]),
        ([[2, 2, 4, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'a', [4, 4, 0, 0]),
        ([[2, 2, 2, 2], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'a', [4, 4, 0, 0]),
        ([[4, 2, 2, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], 'a', [4, 4, 0, 0]),
    ])
    def test_merge_combinations(self, initial, move, expected_first_row):
        """Test various merge combinations"""
        results = run_cpp_game(f"{move}\nq\n", initial)

        merge_board = None
        for stage, board in results:
            if stage == "merge":
                merge_board = board
                break

        assert merge_board is not None
        assert merge_board[0] == expected_first_row
