2.2.3 Deleting Rows
====================

While the storage engine does support row deletion the functionality is
currently not exposed via any API. The feature is expected to land in EventQL
v0.5.x.

In the meantime you can work around this by adding a `deleted` field to your
schemas and settings this to true for row deletion.
