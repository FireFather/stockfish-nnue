/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2020 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <deque>
#include <iostream>
#include <memory> // For std::unique_ptr
#include <string>

#include "bitboard.h"
#include "evaluate.h"

#include "eval/nnue/nnue_accumulator.h"


/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {

  // Copied when making a move
  Key    pawnKey;
  Key    materialKey;
  Value  nonPawnMaterial[COLOR_NB];
  int    castlingRights;
  int    rule50;
  int    pliesFromNull;
  Square epSquare;

  // Not copied when making a move (will be recomputed anyhow)
  Key        key;
  Bitboard   checkersBB;
  Piece      capturedPiece;
  StateInfo* previous;
  Bitboard   blockersForKing[COLOR_NB];
  Bitboard   pinners[COLOR_NB];
  Bitboard   checkSquares[PIECE_TYPE_NB];
  int        repetition;

#if defined(EVAL_NNUE)
  Eval::NNUE::Accumulator accumulator;

   // For management of evaluation value difference calculation
  Eval::DirtyPiece dirtyPiece;
#endif  // defined(EVAL_NNUE)
};


/// A list to keep track of the position states along the setup moves (from the
/// start position to the position just before the search starts). Needed by
/// 'draw by repetition' detection. Use a std::deque because pointers to
/// elements are not invalidated upon list resizing.
typedef std::unique_ptr<std::deque<StateInfo>> StateListPtr;


/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.
class Thread;

// packed sfen
struct PackedSfen { uint8_t data[32]; };

class Position {
public:
  static void init();

  Position() = default;
  Position(const Position&) = delete;
  Position& operator=(const Position&) = delete;

  // FEN string input/output
  Position& set(const std::string& fenStr, bool isChess960, StateInfo* si, Thread* th);
  Position& set(const std::string& code, Color c, StateInfo* si);
  [[nodiscard]] std::string fen() const;

  // Position representation
  [[nodiscard]] Bitboard pieces(PieceType pt) const;
  [[nodiscard]] Bitboard pieces(PieceType pt1, PieceType pt2) const;
  [[nodiscard]] Bitboard pieces(Color c) const;
  [[nodiscard]] Bitboard pieces(Color c, PieceType pt) const;
  [[nodiscard]] Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
  [[nodiscard]] Piece piece_on(Square s) const;
  [[nodiscard]] Square ep_square() const;
  [[nodiscard]] bool empty(Square s) const;
  template<PieceType Pt>
  [[nodiscard]] int count(Color c) const;
  template<PieceType Pt>
  [[nodiscard]] int count() const;
  template<PieceType Pt>
  [[nodiscard]] const Square* squares(Color c) const;
  template<PieceType Pt>
  [[nodiscard]] Square square(Color c) const;
  [[nodiscard]] bool is_on_semiopen_file(Color c, Square s) const;

  // Castling
  [[nodiscard]] CastlingRights castling_rights(Color c) const;
  [[nodiscard]] bool can_castle(CastlingRights cr) const;
  [[nodiscard]] bool castling_impeded(CastlingRights cr) const;
  [[nodiscard]] Square castling_rook_square(CastlingRights cr) const;

  // Checking
  [[nodiscard]] Bitboard checkers() const;
  [[nodiscard]] Bitboard blockers_for_king(Color c) const;
  [[nodiscard]] Bitboard check_squares(PieceType pt) const;
  [[nodiscard]] bool is_discovery_check_on_king(Color c, Move m) const;

  // Attacks to/from a given square
  [[nodiscard]] Bitboard attackers_to(Square s) const;
  [[nodiscard]] Bitboard attackers_to(Square s, Bitboard occupied) const;
  Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const;

  // Properties of moves
  [[nodiscard]] bool legal(Move m) const;
  [[nodiscard]] bool pseudo_legal(Move m) const;
  [[nodiscard]] bool capture(Move m) const;
  [[nodiscard]] bool capture_or_promotion(Move m) const;
  [[nodiscard]] bool gives_check(Move m) const;
  [[nodiscard]] bool advanced_pawn_push(Move m) const;
  [[nodiscard]] Piece moved_piece(Move m) const;
  [[nodiscard]] Piece captured_piece() const;

  // Piece specific
  [[nodiscard]] bool pawn_passed(Color c, Square s) const;
  [[nodiscard]] bool opposite_bishops() const;
  [[nodiscard]] int  pawns_on_same_color_squares(Color c, Square s) const;

  // Doing and undoing moves
  void do_move(Move m, StateInfo& newSt);
  void do_move(Move m, StateInfo& newSt, bool givesCheck);
  void undo_move(Move m);
  void do_null_move(StateInfo& newSt);
  void undo_null_move();

  // Static Exchange Evaluation
  [[nodiscard]] bool see_ge(Move m, Value threshold = VALUE_ZERO) const;

  // Accessing hash keys
  [[nodiscard]] Key key() const;
  [[nodiscard]] Key key_after(Move m) const;
  [[nodiscard]] Key material_key() const;
  [[nodiscard]] Key pawn_key() const;

  // Other properties of the position
  [[nodiscard]] Color side_to_move() const;
  [[nodiscard]] int game_ply() const;
  [[nodiscard]] bool is_chess960() const;
  [[nodiscard]] Thread* this_thread() const;
  [[nodiscard]] bool is_draw(int ply) const;
  [[nodiscard]] bool has_game_cycle(int ply) const;
  [[nodiscard]] bool has_repeated() const;
  [[nodiscard]] int rule50_count() const;
  [[nodiscard]] Score psq_score() const;
  [[nodiscard]] Value non_pawn_material(Color c) const;
  [[nodiscard]] Value non_pawn_material() const;

  // Position consistency check, for debugging
  [[nodiscard]] bool pos_is_ok() const;
  void flip();

#if defined(EVAL_NNUE) || defined(EVAL_LEARN)
  // --- StateInfo

  // Returns the StateInfo corresponding to the current situation.
  // For example, if state()->capturedPiece, the pieces captured in the previous phase are stored.
  [[nodiscard]] StateInfo* state() const { return st; }

  // Information such as where and which piece number is used for the evaluation function.
  [[nodiscard]] const Eval::EvalList* eval_list() const { return &evalList; }
#endif  // defined(EVAL_NNUE) || defined(EVAL_LEARN)

#if defined(EVAL_LEARN)
  // --sfenization helper

  // Get the packed sfen. Returns to the buffer specified in the argument.
  // Do not include gamePly in pack.
  void sfen_pack(PackedSfen& sfen) const;

  // �� It is slow to go through sfen, so I made a function to set packed sfen directly.
  // Equivalent to pos.set(sfen_unpack(data),si,th);.
  // If there is a problem with the passed phase and there is an error, non-zero is returned.
  // PackedSfen does not include gamePly so it cannot be restored. If you want to set it, specify it with an argument.
  int set_from_packed_sfen(const PackedSfen& sfen, StateInfo* si, Thread* th, bool mirror = false);

  // Give the board, hand piece, and turn, and return the sfen.
  //static std::string sfen_from_rawdata(Piece board[81], Hand hands[2], Color turn, int gamePly);

  // Returns the position of the ball on the c side.
  [[nodiscard]] Square king_square(const Color c) const { return pieceList[make_piece(c, KING)][0]; }
#endif // EVAL_LEARN

private:
  // Initialization helpers (used while setting up a position)
  void set_castling_right(Color c, Square rfrom);
  void set_state(StateInfo* si) const;
  void set_check_info(StateInfo* si) const;

  // Other helpers
  void put_piece(Piece pc, Square s);
  void remove_piece(Square s);
  void move_piece(Square from, Square to);
  template<bool Do>
  void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

#if defined(EVAL_NNUE)
  // Returns the PieceNumber of the piece in the sq box on the board.
  [[nodiscard]] PieceNumber piece_no_of(Square sq) const;
#endif  // defined(EVAL_NNUE)

  // Data members
  Piece board[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  int pieceCount[PIECE_NB];
  Square pieceList[PIECE_NB][16];
  int index[SQUARE_NB];
  int castlingRightsMask[SQUARE_NB];
  Square castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
  int gamePly;
  Color sideToMove;
  Score psq;
  Thread* thisThread;
  StateInfo* st;
  bool chess960;

#if defined(EVAL_NNUE) || defined(EVAL_LEARN)
  // List of pieces used in the evaluation function
  Eval::EvalList evalList;
#endif  // defined(EVAL_NNUE) || defined(EVAL_LEARN)
};

namespace PSQT {
  extern Score psq[PIECE_NB][SQUARE_NB];
}

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Color Position::side_to_move() const {
  return sideToMove;
}

inline Piece Position::piece_on(const Square s) const {
  assert(is_ok(s));
  return board[s];
}

inline bool Position::empty(const Square s) const {
  return piece_on(s) == NO_PIECE;
}

inline Piece Position::moved_piece(const Move m) const {
  return piece_on(from_sq(m));
}

inline Bitboard Position::pieces(const PieceType pt = ALL_PIECES) const {
  return byTypeBB[pt];
}

inline Bitboard Position::pieces(const PieceType pt1, const PieceType pt2) const {
  return pieces(pt1) | pieces(pt2);
}

inline Bitboard Position::pieces(const Color c) const {
  return byColorBB[c];
}

inline Bitboard Position::pieces(const Color c, const PieceType pt) const {
  return pieces(c) & pieces(pt);
}

inline Bitboard Position::pieces(const Color c, const PieceType pt1, const PieceType pt2) const {
  return pieces(c) & (pieces(pt1) | pieces(pt2));
}

template<PieceType Pt>
int Position::count(const Color c) const {
  return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt>
int Position::count() const {
  return count<Pt>(WHITE) + count<Pt>(BLACK);
}

template<PieceType Pt>
const Square* Position::squares(const Color c) const {
  return pieceList[make_piece(c, Pt)];
}

template<PieceType Pt>
Square Position::square(const Color c) const {
  assert(pieceCount[make_piece(c, Pt)] == 1);
  return squares<Pt>(c)[0];
}

inline Square Position::ep_square() const {
  return st->epSquare;
}

inline bool Position::is_on_semiopen_file(const Color c, const Square s) const {
  return !(pieces(c, PAWN) & file_bb(s));
}

inline bool Position::can_castle(const CastlingRights cr) const {
  return st->castlingRights & cr;
}

inline CastlingRights Position::castling_rights(const Color c) const {
  return c & static_cast<CastlingRights>(st->castlingRights);
}

inline bool Position::castling_impeded(const CastlingRights cr) const {
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);

  return pieces() & castlingPath[cr];
}

inline Square Position::castling_rook_square(const CastlingRights cr) const {
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);

  return castlingRookSquare[cr];
}

inline Bitboard Position::attackers_to(const Square s) const {
  return attackers_to(s, pieces());
}

inline Bitboard Position::checkers() const {
  return st->checkersBB;
}

inline Bitboard Position::blockers_for_king(const Color c) const {
  return st->blockersForKing[c];
}

inline Bitboard Position::check_squares(const PieceType pt) const {
  return st->checkSquares[pt];
}

inline bool Position::is_discovery_check_on_king(const Color c, const Move m) const {
  return st->blockersForKing[c] & from_sq(m);
}

inline bool Position::pawn_passed(const Color c, const Square s) const {
  return !(pieces(~c, PAWN) & passed_pawn_span(c, s));
}

inline bool Position::advanced_pawn_push(const Move m) const {
  return   type_of(moved_piece(m)) == PAWN
        && relative_rank(sideToMove, to_sq(m)) > RANK_5;
}

inline int Position::pawns_on_same_color_squares(const Color c, const Square s) const {
  return popcount(pieces(c, PAWN) & (DarkSquares & s ? DarkSquares : ~DarkSquares));
}

inline Key Position::key() const {
  return st->key;
}

inline Key Position::pawn_key() const {
  return st->pawnKey;
}

inline Key Position::material_key() const {
  return st->materialKey;
}

inline Score Position::psq_score() const {
  return psq;
}

inline Value Position::non_pawn_material(const Color c) const {
  return st->nonPawnMaterial[c];
}

inline Value Position::non_pawn_material() const {
  return non_pawn_material(WHITE) + non_pawn_material(BLACK);
}

inline int Position::game_ply() const {
  return gamePly;
}

inline int Position::rule50_count() const {
  return st->rule50;
}

inline bool Position::opposite_bishops() const {
  return   count<BISHOP>(WHITE) == 1
        && count<BISHOP>(BLACK) == 1
        && opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

inline bool Position::is_chess960() const {
  return chess960;
}

inline bool Position::capture_or_promotion(const Move m) const {
  assert(is_ok(m));
  return type_of(m) != NORMAL ? type_of(m) != CASTLING : !empty(to_sq(m));
}

inline bool Position::capture(const Move m) const {
  assert(is_ok(m));
  // Castling is encoded as "king captures rook"
  return !empty(to_sq(m)) && type_of(m) != CASTLING || type_of(m) == ENPASSANT;
}

inline Piece Position::captured_piece() const {
  return st->capturedPiece;
}

inline Thread* Position::this_thread() const {
  return thisThread;
}

inline void Position::put_piece(const Piece pc, const Square s) {

  board[s] = pc;
  byTypeBB[ALL_PIECES] |= byTypeBB[type_of(pc)] |= s;
  byColorBB[color_of(pc)] |= s;
  index[s] = pieceCount[pc]++;
  pieceList[pc][index[s]] = s;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
  psq += PSQT::psq[pc][s];
}

inline void Position::remove_piece(const Square s) {

  // WARNING: This is not a reversible operation. If we remove a piece in
  // do_move() and then replace it in undo_move() we will put it at the end of
  // the list and not in its original place, it means index[] and pieceList[]
  // are not invariant to a do_move() + undo_move() sequence.
  const Piece pc = board[s];
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[type_of(pc)] ^= s;
  byColorBB[color_of(pc)] ^= s;
  /* board[s] = NO_PIECE;  Not needed, overwritten by the capturing one */
  const Square lastSquare = pieceList[pc][--pieceCount[pc]];
  index[lastSquare] = index[s];
  pieceList[pc][index[lastSquare]] = lastSquare;
  pieceList[pc][pieceCount[pc]] = SQ_NONE;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
  psq -= PSQT::psq[pc][s];
}

inline void Position::move_piece(const Square from, const Square to) {

  // index[from] is not updated and becomes stale. This works as long as index[]
  // is accessed just by known occupied squares.
  const Piece pc = board[from];
  const Bitboard fromTo = from | to;
  byTypeBB[ALL_PIECES] ^= fromTo;
  byTypeBB[type_of(pc)] ^= fromTo;
  byColorBB[color_of(pc)] ^= fromTo;
  board[from] = NO_PIECE;
  board[to] = pc;
  index[to] = index[from];
  pieceList[pc][index[to]] = to;
  psq += PSQT::psq[pc][to] - PSQT::psq[pc][from];
}

inline void Position::do_move(const Move m, StateInfo& newSt) {
  do_move(m, newSt, gives_check(m));
}

#endif // #ifndef POSITION_H_INCLUDED
