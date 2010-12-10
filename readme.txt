$Id: 


current status: 2010-12-10


feature_tables 1.x for Drupal 6.x
-----------------------------------

The features_tables module exposes entire database tables declared via the drupal schema api as components to features.
This enables you to store and deploy _whole_ database _tables_ in code via features.
This is meant to be used as a last means for modules, which store site buildling _configuration_ in dedicated tables but do not come with inherent features/exportables-support, to work with features.


Examples
--------

- allow your features to store configuration of Drupal 6s

  {filters}          and
  {filter_formats}   along with
  {wysiwyg}          module (all with numeric IDs and therefore without feature support)

- store

  {languages}        configuration of your site in a master feature

- {blocks} etc. 

- any other tables that _only_ store moderate amounts of configuration.


Quirks
------

For some tables/modules it might not suffice to just store the raw table data, even in cases where that itself is clean. One example we came upon is storing the {language} settings in a feature via features_tables, which works nice - but oddly requires you to just save the languages-settings-dialog once the feature has been reverted/rebuilt. Meaning: You may encounter that additional steps are necessary (To prepare and react cleanly to feature_tables imports/exports you have a hook.based api at hand, see resp. section below).


WARNING - DISCLAIMERS - KEEP IN MIND
------------------------------------

Storing whole tables in features is by no means what is intended by features module and certainly not the best way to use features.
Features_tables is intended to be used only if no other means (like adding proper exportables/features support to a module) are viable or seem to be blocked or overly delayed - or simply technically not possible (e.g. obstructed by numerical ids). This is the case for exmaple with {filters} and {filter_formats} tables in Drupal 6.

Feature_tables IS definitely an EVIL MODULE. But it can nevertheless be used for good ends if carefully administrered, just as other evil things [todo for future co-maintainers: insert hilariously funny and odd examples with some political twinkle].    

No questions, the best way to enable modules without inherent features support to be usable with features is add a proper module-specific expoertables-/features-support by yourself - only sometimes ...
- you don't have the knowledge about the module in question or about feature api
  (well, in that case you probably shouldn't use this module, either...)
- you don't have the time (a pity, but happens in real world)
- there is simply no good elegant way to achieve that - or no way at all

As we think, a site should be living cleanly in code. There should be no blank spots on this map, and it should not cost you hours or days to fix shortcomings of other modules or architectures. This is why we created this module.

So, while features_tables is not the pure and proper way to go, it may be helpful in some cases.

However:

You should use features_tables strictly only on small tables that story solely development/site-buidling configuration in moderate amounts (a few or a few dozens of few-byte records, not hundreds or thounsands of records), and that do not get changed in any (configuration-wise significant) way by live operation.
                          
You should use features_tables only, if you know exactly of the evils you are going tp deal with!

You should use features_tables only in version controlled development environment, and _never_ if you site-build directly on a live system without source code version control!

You should use features_tables only as a fast or last measure!

You should use features_tables only if you know how features and the respective tables work!

In any circumstances use this module on your own risk!

Features_tables is in use on D6 development and production sites, but as the (rebellic) youth it is, you should use it with great care (if at all, as explained above)!


Installation
------------        

Just install and enable the module, and you will see your tables (those declared via schema-api) within Features as component/export options "Database tables", which you can select to include in the feature of your choice.


API
---

To alter imported/exported data or execute preparatory or subsequent actions, you can implement several hooks:
(for details see bottom section of features_tables.databasetables.inc)

hook_features_tables_export_preprocess
 - Called just before generating serialized export data from the database

hook_features_tables_export_postprocess
 - Called right after generating serialized export data from the database

hook_features_tables_export_row_alter
 - Called to alter a record/row retrieved from the database right before it is exported to a feature

hook_features_tables_import_preprocess
 - Called just before importing serialized export data from a feature into the database

hook_features_tables_import_postprocess
 - Called right after importing serialized export data from a feature to the database

hook_features_tables_import_row_alter
 - Called to alter a record/row retrieved from a feature just before it is imported into the databsase.


Drupal 7
--------

There will be a Drupal 7 Version of features.module

Drupal 7 core solves many issues related to comprehensive use of features (like using machine names instead of numerical ids in some areas), and many module without feature support yet are likely to get on the features bandwagon. However, this module can still make sense and be helpful in some cases, and porting it shouldn't be hard, so features_tables will get ported to Drupal 7 soon.


Sponsors
--------

Development of features_tables.module is supported and sponsored by Wunderkraut.net


Maintainers
-----------
- danielnolde (Daniel Nolde from Wunderkraut.net)

