# Communication protocol

0. CBOR in request/response body

1. PUT ships to /start, receive UUID & PlayerField

2. /status:
   0. http::unathorized: UUID is expired or wrong
   1. http::locked, empty body -> wait for an enemy
   2. http::locked, body: {status: 0} -> wait for the enemy's next move
   3. http::ok, body: {status: 0} -> PATCH first move to /shoot
   4. http::ok, body: {status: 0, enemy_move} -> display enemy move
      && if not game over then PATCH new move to /shoot
   5. http::ok, body: {status:  1} -> You won!
   6. http::ok, body: {status: -1} -> Game over!

3. If connection is lost, then GET /field:
   1. http::unauthorized: UUID is expired or wrong
   2. http::ok, body: PlayerField
