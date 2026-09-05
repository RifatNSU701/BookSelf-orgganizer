# Database

The application uses SQLite for local persistent storage.

## Layout

- `migrations/001_initial.sql` defines the initial schema.
- `bookshelf.db` is a runtime database and must not be committed.

## Migration policy

Database changes are versioned through numbered SQL migrations. A future application startup layer will record the applied migration version and execute pending migrations inside transactions.

## Data model

The initial schema stores book identity, metadata, shelf placement, availability status, and timestamps. Indexes support common title, author, genre, and year queries.
